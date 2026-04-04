from __future__ import annotations

import os
import subprocess
from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .models import ToolExecutionResult, ToolPlan, ToolSpec


SAFE_EXECUTION_RISKS = {"L0_readonly", "L1_low_risk_exec"}


def build_tool_registry(root: Path) -> list[ToolSpec]:
    script_root = root / "资料" / "imxSoem-motion-control"
    return [
        ToolSpec(
            tool_id="SCRIPT-004",
            name="probe_can_heartbeat",
            description="Collect read-only CAN heartbeat and axis status snapshots.",
            risk_level="L1_low_risk_exec",
            source_script=str(script_root / "tmp_probe_can_heartbeat.py"),
            executor="wsl_python",
            default_env={"POLL_COUNT": "8", "POLL_INTERVAL_SECONDS": "1.0"},
            tags=["heartbeat", "snapshot", "readonly", "axis_status"],
        ),
        ToolSpec(
            tool_id="SCRIPT-003",
            name="axis_probe",
            description="Single-axis probe that writes motion parameters then observes feedback.",
            risk_level="L2_high_risk_exec",
            source_script=str(script_root / "tmp_axis_probe.py"),
            executor="wsl_python",
            tags=["axis", "probe", "acc_dec", "motion"],
        ),
        ToolSpec(
            tool_id="SCRIPT-002",
            name="axis_command_and_probe",
            description="Dual-axis command-and-probe flow for coordinated debugging.",
            risk_level="L2_high_risk_exec",
            source_script=str(script_root / "tmp_axis_command_and_probe.py"),
            executor="wsl_python",
            tags=["dual_axis", "motion", "command"],
        ),
        ToolSpec(
            tool_id="SCRIPT-001",
            name="axis1_delay_probe",
            description="Drive axis1 in speed mode and observe delayed response behavior.",
            risk_level="L2_high_risk_exec",
            source_script=str(script_root / "tmp_axis1_delay_probe.py"),
            executor="wsl_python",
            tags=["axis1", "delay", "motion"],
        ),
        ToolSpec(
            tool_id="SCRIPT-005",
            name="verify_acc_dec",
            description="Verify acceleration/deceleration write-through and encoder changes.",
            risk_level="L2_high_risk_exec",
            source_script=str(script_root / "tmp_verify_acc_dec.py"),
            executor="wsl_python",
            tags=["acc_dec", "verify", "motion_param_latch"],
        ),
    ]


def list_tools(root: Path) -> list[dict[str, object]]:
    return [asdict(tool) for tool in build_tool_registry(root)]


def plan_tool_request(root: Path, request: str, *, tool_id: str | None = None) -> ToolPlan:
    diagnosis = analyze_text(request, mode="auto")
    registry = {tool.tool_id: tool for tool in build_tool_registry(root)}
    tool = registry.get(tool_id) if tool_id else _select_tool(request, diagnosis, registry)

    if diagnosis.should_refuse:
        return ToolPlan(
            request=request,
            summary=diagnosis.summary,
            tool_id=tool.tool_id if tool else None,
            tool_name=tool.name if tool else None,
            risk_level=diagnosis.risk_level,
            allowed_to_execute=False,
            requires_confirmation=True,
            should_refuse=True,
            command_preview=_build_command_preview(root, tool) if tool else [],
            reason="Request crosses the current L2 execution boundary and must stay manual.",
            evidence=diagnosis.evidence,
        )

    if tool is None:
        return ToolPlan(
            request=request,
            summary=diagnosis.summary,
            tool_id=None,
            tool_name=None,
            risk_level=diagnosis.risk_level,
            allowed_to_execute=False,
            requires_confirmation=False,
            should_refuse=False,
            command_preview=[],
            reason="No matching tool is registered yet. Keep this as analysis-only for now.",
            evidence=diagnosis.evidence,
        )

    requires_confirmation = tool.risk_level not in SAFE_EXECUTION_RISKS
    allowed_to_execute = tool.risk_level in SAFE_EXECUTION_RISKS and Path(tool.source_script).exists()
    reason = _tool_plan_reason(tool, diagnosis, allowed_to_execute, requires_confirmation)
    return ToolPlan(
        request=request,
        summary=diagnosis.summary,
        tool_id=tool.tool_id,
        tool_name=tool.name,
        risk_level=tool.risk_level,
        allowed_to_execute=allowed_to_execute,
        requires_confirmation=requires_confirmation,
        should_refuse=False,
        command_preview=_build_command_preview(root, tool),
        reason=reason,
        evidence=diagnosis.evidence,
    )


def run_tool_request(
    root: Path,
    request: str,
    *,
    tool_id: str | None = None,
    execute: bool = False,
    timeout_seconds: int = 20,
) -> dict[str, object]:
    plan = plan_tool_request(root, request, tool_id=tool_id)
    payload: dict[str, object] = {"plan": asdict(plan)}
    if not execute:
        payload["execution"] = asdict(
            ToolExecutionResult(
                tool_id=plan.tool_id or "",
                command=plan.command_preview,
                executed=False,
                returncode=None,
                stdout="",
                stderr="Execution skipped. Re-run with --execute to actually invoke the tool.",
                risk_level=plan.risk_level,
            )
        )
        return payload

    if not plan.tool_id or not plan.allowed_to_execute or plan.should_refuse:
        payload["execution"] = asdict(
            ToolExecutionResult(
                tool_id=plan.tool_id or "",
                command=plan.command_preview,
                executed=False,
                returncode=None,
                stdout="",
                stderr=plan.reason,
                risk_level=plan.risk_level,
            )
        )
        return payload

    registry = {tool.tool_id: tool for tool in build_tool_registry(root)}
    tool = registry[plan.tool_id]
    command = _build_command_preview(root, tool)
    env = os.environ.copy()
    env.update(tool.default_env)
    try:
        completed = subprocess.run(
            command,
            cwd=str(root),
            env=env,
            capture_output=True,
            timeout=timeout_seconds,
        )
        execution = ToolExecutionResult(
            tool_id=tool.tool_id,
            command=command,
            executed=True,
            returncode=completed.returncode,
            stdout=_decode_output(completed.stdout)[-4000:],
            stderr=_decode_output(completed.stderr)[-4000:],
            risk_level=tool.risk_level,
        )
    except Exception as exc:  # pragma: no cover - defensive wrapper
        execution = ToolExecutionResult(
            tool_id=tool.tool_id,
            command=command,
            executed=True,
            returncode=None,
            stdout="",
            stderr=str(exc),
            risk_level=tool.risk_level,
        )
    payload["execution"] = asdict(execution)
    return payload


def _select_tool(request: str, diagnosis, registry: dict[str, ToolSpec]) -> ToolSpec | None:
    normalized = request.lower()
    if any(token in normalized for token in ["heartbeat", "快照", "状态快照", "错误码", "状态字", "编码器", "只读"]):
        return registry.get("SCRIPT-004")
    if "tmp_probe_can_heartbeat.py" in normalized:
        return registry.get("SCRIPT-004")
    if "tmp_verify_acc_dec.py" in normalized or ("acc" in normalized and "dec" in normalized):
        return registry.get("SCRIPT-005")
    if "tmp_axis_command_and_probe.py" in normalized or "双轴" in request:
        return registry.get("SCRIPT-002")
    if "tmp_axis1_delay_probe.py" in normalized:
        return registry.get("SCRIPT-001")
    if "tmp_axis_probe.py" in normalized or diagnosis.issue_category == "motion_param_latch":
        return registry.get("SCRIPT-003")
    if diagnosis.issue_category == "verification_tooling":
        return registry.get("SCRIPT-004")
    return None


def _tool_plan_reason(tool: ToolSpec, diagnosis, allowed_to_execute: bool, requires_confirmation: bool) -> str:
    if allowed_to_execute:
        return f"Matched {tool.tool_id} because it stays within the {tool.risk_level} boundary and fits the current request."
    if requires_confirmation:
        return f"{tool.tool_id} matches the request, but it is classified as {tool.risk_level} and stays blocked pending manual confirmation."
    if not Path(tool.source_script).exists():
        return f"{tool.tool_id} matches the request, but the backing script path is missing."
    return f"{tool.tool_id} matches the request, but current policy keeps it blocked."


def _build_command_preview(root: Path, tool: ToolSpec) -> list[str]:
    script_path = Path(tool.source_script)
    if tool.executor == "wsl_python":
        return ["wsl.exe", "python3", _to_wsl_path(script_path), *tool.default_args]
    if tool.executor == "python":
        return ["python", str(script_path), *tool.default_args]
    return [str(script_path), *tool.default_args]


def _to_wsl_path(path: Path) -> str:
    drive = path.drive.rstrip(":").lower()
    tail = path.as_posix().split(":/", 1)[1] if ":/" in path.as_posix() else path.as_posix()
    return f"/mnt/{drive}/{tail}"


def _decode_output(payload: bytes) -> str:
    if payload.count(b"\x00") > max(4, len(payload) // 8):
        try:
            return payload.decode("utf-16le")
        except UnicodeDecodeError:
            pass
    for encoding in ("utf-8", "gbk"):
        try:
            return payload.decode(encoding)
        except UnicodeDecodeError:
            continue
    return payload.decode("utf-8", errors="replace")
