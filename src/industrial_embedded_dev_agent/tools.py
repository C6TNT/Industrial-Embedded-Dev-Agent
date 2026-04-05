from __future__ import annotations

import os
import re
import subprocess
from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .models import ToolExecutionResult, ToolPlan, ToolSpec


SAFE_EXECUTION_RISKS = {"L0_readonly", "L1_low_risk_exec"}
WSL_STUB_LIBRARY = "/home/librobot.so.1.0.0"
WSL_STUB_FLAG = ".ieda_wsl_stub_enabled"

READONLY_TOKENS = ["状态字", "错误码", "编码器", "只读"]
SNAPSHOT_TOKENS = ["快照", "采集", "heartbeat"]


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


def inspect_wsl_environment(root: Path) -> dict[str, object]:
    stub_mode_enabled = (root / WSL_STUB_FLAG).exists()
    stub_library_present = _command_success(["wsl.exe", "bash", "-lc", f"test -f {WSL_STUB_LIBRARY}"])
    return {
        "wsl_available": _command_success(["wsl.exe", "bash", "-lc", "true"]),
        "python3_available": _command_success(["wsl.exe", "bash", "-lc", "command -v python3 >/dev/null 2>&1"]),
        "gcc_available": _command_success(["wsl.exe", "bash", "-lc", "command -v gcc >/dev/null 2>&1"]),
        "execution_mode": _resolve_execution_mode(root, stub_library_present=stub_library_present),
        "stub_mode_enabled": stub_mode_enabled,
        "real_mode_ready": (not stub_mode_enabled) and stub_library_present,
        "stub_library_path": WSL_STUB_LIBRARY,
        "stub_library_present": stub_library_present,
        "stub_library_file_info": _capture_text(
            ["wsl.exe", "bash", "-lc", f"if test -f {WSL_STUB_LIBRARY}; then file {WSL_STUB_LIBRARY}; fi"]
        ).strip(),
    }


def get_execution_mode(root: Path) -> dict[str, object]:
    environment = inspect_wsl_environment(root)
    mode = environment["execution_mode"]
    summaries = {
        "stub": "Current tool execution runs through the no-hardware stub wrapper.",
        "real": "Current tool execution runs against the real WSL librobot path.",
        "real_unavailable": "Current tool execution expects the real WSL librobot path, but that path is not ready yet.",
    }
    return {"mode": mode, "summary": summaries[mode], "environment": environment}


def setup_wsl_stub_environment(root: Path) -> dict[str, object]:
    script_path = root / "scripts" / "setup_wsl_stub.ps1"
    command = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script_path)]
    completed = subprocess.run(command, cwd=str(root), capture_output=True, text=True)
    return {
        "command": command,
        "returncode": completed.returncode,
        "stdout": completed.stdout[-4000:],
        "stderr": completed.stderr[-4000:],
        "environment": inspect_wsl_environment(root),
    }


def disable_wsl_stub_environment(root: Path) -> dict[str, object]:
    flag_path = root / WSL_STUB_FLAG
    if flag_path.exists():
        flag_path.unlink()
        message = f"Disabled stub mode by removing {flag_path.name}"
    else:
        message = "Stub mode was already disabled."
    return {"status": "ok", "message": message, "environment": inspect_wsl_environment(root)}


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
            command_preview=_build_command_preview(tool, root=root) if tool else [],
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
    reason = _tool_plan_reason(tool, request, allowed_to_execute, requires_confirmation, effective_risk, root)
    return ToolPlan(
        request=request,
        summary=effective_summary,
        tool_id=tool.tool_id,
        tool_name=tool.name,
        risk_level=effective_risk,
        allowed_to_execute=allowed_to_execute,
        requires_confirmation=requires_confirmation,
        should_refuse=False,
        command_preview=_build_command_preview(tool, root=root),
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
    command = _build_command_preview(tool, root=root)
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
    if any(token in normalized for token in ["heartbeat", "snapshot", "readonly"]) or any(token in request for token in READONLY_TOKENS + SNAPSHOT_TOKENS):
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
    root: Path,
) -> str:
    normalized = request.lower()
    current_mode = _resolve_execution_mode(root)
    mode_note = f" Current execution mode: {current_mode}."

    if tool.tool_id == "SCRIPT-004" and any(token in request for token in READONLY_TOKENS):
        return "Matched SCRIPT-004 because this is a read-only collection request for statusword / error code / encoder." + mode_note
    if tool.tool_id == "SCRIPT-004" and (
        any(token in normalized for token in ["heartbeat", "snapshot"]) or any(token in request for token in SNAPSHOT_TOKENS)
    ):
        return "Matched SCRIPT-004 because it is the low-risk snapshot tool for heartbeat and axis-state collection." + mode_note
    if allowed_to_execute:
        return f"Matched {tool.tool_id} because it stays within the {effective_risk} boundary and fits the current request." + mode_note
    if requires_confirmation:
        return f"{tool.tool_id} matches the request, but it is classified as {effective_risk} and stays blocked pending manual confirmation." + mode_note
    if not Path(tool.source_script).exists():
        return f"{tool.tool_id} matches the request, but the backing script path is missing."
    return f"{tool.tool_id} matches the request, but current policy keeps it blocked." + mode_note


def _build_command_preview(tool: ToolSpec, *, root: Path) -> list[str]:
    script_path = Path(tool.source_script)
    if tool.executor == "wsl_python":
        return _build_wsl_python_command(script_path, tool.default_args, root=root)
    if tool.executor == "python":
        return ["python", str(script_path), *tool.default_args]
    return [str(script_path), *tool.default_args]


def _command_success(command: list[str]) -> bool:
    completed = subprocess.run(command, capture_output=True)
    return completed.returncode == 0


def _capture_text(command: list[str]) -> str:
    completed = subprocess.run(command, capture_output=True)
    return _decode_output(completed.stdout)


def _build_wsl_python_command(script_path: Path, default_args: list[str], *, root: Path) -> list[str]:
    if _should_use_wsl_stub(root):
        wrapper_path = root / "scripts" / "wsl" / "run_with_stub.py"
        return ["wsl.exe", "python3", _to_wsl_path(wrapper_path), _to_wsl_path(script_path), *default_args]
    return ["wsl.exe", "python3", _to_wsl_path(script_path), *default_args]


def _should_use_wsl_stub(root: Path) -> bool:
    return (root / WSL_STUB_FLAG).exists()


def _resolve_execution_mode(root: Path, *, stub_library_present: bool | None = None) -> str:
    if _should_use_wsl_stub(root):
        return "stub"
    library_ready = (
        stub_library_present
        if stub_library_present is not None
        else _command_success(["wsl.exe", "bash", "-lc", f"test -f {WSL_STUB_LIBRARY}"])
    )
    if library_ready:
        return "real"
    return "real_unavailable"


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
    if tool.tool_id == "SCRIPT-004" and any(token in request for token in READONLY_TOKENS):
        return "L0_readonly"
    if tool.tool_id == "SCRIPT-004":
        return "L1_low_risk_exec"
    return tool.risk_level


def _effective_tool_summary(request: str, diagnosis, tool: ToolSpec) -> str:
    normalized = request.lower()
    if tool.tool_id == "SCRIPT-004" and any(token in request for token in READONLY_TOKENS):
        return "这是只读请求，可先读取状态字、错误码和编码器，确认当前链路是否可读。"
    if tool.tool_id == "SCRIPT-004" and (
        "heartbeat" in normalized or "snapshot" in normalized or any(token in request for token in SNAPSHOT_TOKENS)
    ):
        return "这是低风险状态采集请求，适合先跑状态快照和 heartbeat 采集，再汇总结论。"
    return diagnosis.summary


def _parse_execution_output(tool_id: str, stdout: str, stderr: str, returncode: int | None) -> dict[str, object]:
    if tool_id == "SCRIPT-004":
        return _parse_probe_can_heartbeat(stdout, stderr, returncode)
    return _parse_generic_output(stdout, stderr, returncode)


def _parse_probe_can_heartbeat(stdout: str, stderr: str, returncode: int | None) -> dict[str, object]:
    if returncode not in (0, None):
        if WSL_STUB_LIBRARY in stderr:
            return {
                "status": "environment_error",
                "summary": "The WSL librobot path is missing, so the heartbeat probe cannot load its transport library.",
                "error_type": "missing_shared_library",
            }
        return {
            "status": "execution_failed",
            "summary": "The heartbeat probe failed. Inspect stderr for the primary failure.",
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

    chain_ok = bool(open_rpmsg_match and int(open_rpmsg_match.group(1)) == 0)
    axis_health: dict[str, str] = {}
    observations: list[str] = []
    summary_parts: list[str] = []

    if open_rpmsg_match:
        summary_parts.append(f"OpenRpmsg={open_rpmsg_match.group(1)}")
    if poll_numbers:
        summary_parts.append(f"polls={max(poll_numbers)}")

    for axis, payload in sorted(axes.items(), key=lambda item: int(item[0])):
        state = _describe_axis_snapshot(payload)
        axis_health[axis] = state
        observations.append(
            f"axis{axis}: {state}, statusCode={payload['status_code']}, axisStatus={payload['axis_status']}, encoder={payload['encoder']}"
        )
        summary_parts.append(
            f"axis{axis}: statusCode={payload['status_code']} axisStatus={payload['axis_status']} encoder={payload['encoder']}"
        )

    next_action = (
        "The transport looks readable. Next, compare these snapshots against vendor-tool or SDO readback on the real bench."
        if chain_ok and axes
        else "Re-check the WSL transport path and runtime setup before treating this as a valid bench snapshot."
    )

    return {
        "status": "ok",
        "summary": "; ".join(summary_parts) if summary_parts else "Heartbeat probe completed, but no structured axis snapshot was parsed.",
        "open_rpmsg_return": int(open_rpmsg_match.group(1)) if open_rpmsg_match else None,
        "poll_count": max(poll_numbers) if poll_numbers else 0,
        "transport_state": "readable" if chain_ok else "unverified",
        "axes": axes,
        "axis_health": axis_health,
        "observations": observations,
        "next_action": next_action,
    }


def _parse_generic_output(stdout: str, stderr: str, returncode: int | None) -> dict[str, object]:
    if returncode in (0, None):
        first_line = next((line.strip() for line in stdout.splitlines() if line.strip()), "")
        return {"status": "ok", "summary": first_line or "Tool execution completed."}
    first_error = next((line.strip() for line in stderr.splitlines() if line.strip()), "Tool execution failed.")
    return {"status": "execution_failed", "summary": first_error}


def _describe_axis_snapshot(payload: dict[str, int]) -> str:
    if payload["error_code"] != 0:
        return "drive reports non-zero error code"
    if payload["status_code"] == 33 and payload["axis_status"] in {4097, 4660, 4664}:
        return "idle but readable"
    if payload["status_code"] == 39 or payload["axis_status"] >= 8193:
        return "appears enabled or moving"
    return "readback received, but state needs manual interpretation"
