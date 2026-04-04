from __future__ import annotations

import os
import re
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
            command_preview=_build_command_preview(tool) if tool else [],
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

    effective_risk = _effective_tool_risk(request, tool)
    effective_summary = _effective_tool_summary(request, diagnosis, tool)
    requires_confirmation = effective_risk not in SAFE_EXECUTION_RISKS
    allowed_to_execute = effective_risk in SAFE_EXECUTION_RISKS and Path(tool.source_script).exists()
    reason = _tool_plan_reason(tool, request, allowed_to_execute, requires_confirmation, effective_risk)
    return ToolPlan(
        request=request,
        summary=effective_summary,
        tool_id=tool.tool_id,
        tool_name=tool.name,
        risk_level=effective_risk,
        allowed_to_execute=allowed_to_execute,
        requires_confirmation=requires_confirmation,
        should_refuse=False,
        command_preview=_build_command_preview(tool),
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
                parsed_output={"status": "skipped"},
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
                parsed_output={"status": "blocked", "reason": plan.reason},
            )
        )
        return payload

    registry = {tool.tool_id: tool for tool in build_tool_registry(root)}
    tool = registry[plan.tool_id]
    command = _build_command_preview(tool)
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
        stdout_text = _decode_output(completed.stdout)[-4000:]
        stderr_text = _decode_output(completed.stderr)[-4000:]
        execution = ToolExecutionResult(
            tool_id=tool.tool_id,
            command=command,
            executed=True,
            returncode=completed.returncode,
            stdout=stdout_text,
            stderr=stderr_text,
            risk_level=plan.risk_level,
            parsed_output=_parse_execution_output(tool.tool_id, stdout_text, stderr_text, completed.returncode),
        )
    except Exception as exc:  # pragma: no cover
        execution = ToolExecutionResult(
            tool_id=tool.tool_id,
            command=command,
            executed=True,
            returncode=None,
            stdout="",
            stderr=str(exc),
            risk_level=plan.risk_level,
            parsed_output={"status": "execution_error", "error": str(exc)},
        )

    payload["execution"] = asdict(execution)
    return payload


def _select_tool(request: str, diagnosis, registry: dict[str, ToolSpec]) -> ToolSpec | None:
    normalized = request.lower()
    if any(token in normalized for token in ["heartbeat", "snapshot", "readonly"]) or any(
        token in request for token in ["快照", "状态快照", "错误码", "状态字", "编码器", "只读"]
    ):
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


def _tool_plan_reason(
    tool: ToolSpec,
    request: str,
    allowed_to_execute: bool,
    requires_confirmation: bool,
    effective_risk: str,
) -> str:
    normalized = request.lower()
    if tool.tool_id == "SCRIPT-004" and any(token in request for token in ["状态字", "编码器", "错误码", "只读"]):
        return "Matched SCRIPT-004 because this is a 只读 request for 状态字/错误码/编码器 collection."
    if tool.tool_id == "SCRIPT-004" and (
        any(token in normalized for token in ["heartbeat", "snapshot"]) or "快照" in request or "采集" in request
    ):
        return "Matched SCRIPT-004 because it is a 低风险采集 tool for 状态快照 and heartbeat snapshots."
    if allowed_to_execute:
        return f"Matched {tool.tool_id} because it stays within the {effective_risk} boundary and fits the current request."
    if requires_confirmation:
        return f"{tool.tool_id} matches the request, but it is classified as {effective_risk} and stays blocked pending manual confirmation."
    if not Path(tool.source_script).exists():
        return f"{tool.tool_id} matches the request, but the backing script path is missing."
    return f"{tool.tool_id} matches the request, but current policy keeps it blocked."


def _build_command_preview(tool: ToolSpec) -> list[str]:
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


def _effective_tool_risk(request: str, tool: ToolSpec) -> str:
    if tool.tool_id == "SCRIPT-004" and any(token in request for token in ["状态字", "编码器", "错误码", "只读"]):
        return "L0_readonly"
    if tool.tool_id == "SCRIPT-004":
        return "L1_low_risk_exec"
    return tool.risk_level


def _effective_tool_summary(request: str, diagnosis, tool: ToolSpec) -> str:
    normalized = request.lower()
    if tool.tool_id == "SCRIPT-004" and any(token in request for token in ["状态字", "编码器", "错误码", "只读"]):
        return "这是只读请求，可先读取状态字、错误码和编码器，确认链路是否还活着。"
    if tool.tool_id == "SCRIPT-004" and (
        "heartbeat" in normalized or "snapshot" in normalized or "快照" in request or "采集" in request
    ):
        return "这是低风险采集请求，适合运行状态快照工具后汇总结果。"
    return diagnosis.summary


def _parse_execution_output(tool_id: str, stdout: str, stderr: str, returncode: int | None) -> dict[str, object]:
    if tool_id == "SCRIPT-004":
        return _parse_probe_can_heartbeat(stdout, stderr, returncode)
    return _parse_generic_output(stdout, stderr, returncode)


def _parse_probe_can_heartbeat(stdout: str, stderr: str, returncode: int | None) -> dict[str, object]:
    if returncode not in (0, None):
        if "/home/librobot.so.1.0.0" in stderr:
            return {
                "status": "environment_error",
                "summary": "WSL 侧缺少 /home/librobot.so.1.0.0，当前无法真正执行 heartbeat 探针。",
                "error_type": "missing_shared_library",
            }
        return {
            "status": "execution_failed",
            "summary": "heartbeat 探针执行失败，请查看 stderr。",
        }

    open_rpmsg_match = re.search(r"OpenRpmsg\s+(-?\d+)", stdout)
    poll_numbers = [int(match) for match in re.findall(r"POLL\s+(\d+)", stdout)]
    axis_matches = re.findall(
        r"AXIS\s+(\d+)\s+errorRet=(-?\d+)\s+errorCode=(-?\d+)\s+statusRet=(-?\d+)\s+statusCode=(-?\d+)\s+axisRet=(-?\d+)\s+axisStatus=(-?\d+)\s+encoderRet=(-?\d+)\s+encoder=(-?\d+)",
        stdout,
    )

    axes: dict[str, dict[str, int]] = {}
    for match in axis_matches:
        axis, error_ret, error_code, status_ret, status_code, axis_ret, axis_status, encoder_ret, encoder = match
        axes[axis] = {
            "error_ret": int(error_ret),
            "error_code": int(error_code),
            "status_ret": int(status_ret),
            "status_code": int(status_code),
            "axis_ret": int(axis_ret),
            "axis_status": int(axis_status),
            "encoder_ret": int(encoder_ret),
            "encoder": int(encoder),
        }

    summary_parts: list[str] = []
    if open_rpmsg_match:
        summary_parts.append(f"OpenRpmsg={open_rpmsg_match.group(1)}")
    if poll_numbers:
        summary_parts.append(f"polls={max(poll_numbers)}")
    for axis, payload in sorted(axes.items(), key=lambda item: int(item[0])):
        summary_parts.append(
            f"axis{axis}: statusCode={payload['status_code']} axisStatus={payload['axis_status']} encoder={payload['encoder']}"
        )

    return {
        "status": "ok",
        "summary": "; ".join(summary_parts) if summary_parts else "heartbeat 探针已执行，但未解析到结构化输出。",
        "open_rpmsg_return": int(open_rpmsg_match.group(1)) if open_rpmsg_match else None,
        "poll_count": max(poll_numbers) if poll_numbers else 0,
        "axes": axes,
    }


def _parse_generic_output(stdout: str, stderr: str, returncode: int | None) -> dict[str, object]:
    if returncode in (0, None):
        first_line = next((line.strip() for line in stdout.splitlines() if line.strip()), "")
        return {"status": "ok", "summary": first_line or "Tool execution completed."}
    first_error = next((line.strip() for line in stderr.splitlines() if line.strip()), "Tool execution failed.")
    return {"status": "execution_failed", "summary": first_error}
