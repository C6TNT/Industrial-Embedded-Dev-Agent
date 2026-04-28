from __future__ import annotations

import json
import re
import subprocess
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path


DEFAULT_BOARD_HOST = "192.168.3.33"
DEFAULT_BOARD_USER = "root"
DEFAULT_SSH_PORT = 22

BLOCKED_REMOTE_TOKENS = [
    "--start-bus",
    "--stop-bus",
    "start-bus",
    "stop-bus",
    "0x86",
    "0x41f1",
    "echo start",
    "echo stop",
    "tee /sys/class/remoteproc",
    ">/sys/class/remoteproc",
    "> /sys/class/remoteproc",
    "reboot",
    "poweroff",
    "shutdown",
    "flash",
    "dd ",
    "write output",
    "io output",
]


@dataclass(frozen=True)
class BoardTarget:
    host: str = DEFAULT_BOARD_HOST
    user: str = DEFAULT_BOARD_USER
    port: int = DEFAULT_SSH_PORT


def board_status(
    *,
    host: str = DEFAULT_BOARD_HOST,
    user: str = DEFAULT_BOARD_USER,
    port: int = DEFAULT_SSH_PORT,
    execute: bool = False,
    timeout_seconds: int = 8,
) -> dict[str, object]:
    """Check board reachability and required files without changing board state."""

    return _run_board_probe(
        "board_status",
        _target(host, user, port),
        _board_status_script(),
        execute=execute,
        timeout_seconds=timeout_seconds,
    )


def rpmsg_health(
    *,
    host: str = DEFAULT_BOARD_HOST,
    user: str = DEFAULT_BOARD_USER,
    port: int = DEFAULT_SSH_PORT,
    execute: bool = False,
    timeout_seconds: int = 8,
) -> dict[str, object]:
    """Inspect RPMsg devices and endpoint discovery without bus control."""

    return _run_board_probe(
        "rpmsg_health",
        _target(host, user, port),
        _rpmsg_health_script(),
        execute=execute,
        timeout_seconds=timeout_seconds,
    )


def m7_health(
    *,
    host: str = DEFAULT_BOARD_HOST,
    user: str = DEFAULT_BOARD_USER,
    port: int = DEFAULT_SSH_PORT,
    execute: bool = False,
    timeout_seconds: int = 8,
) -> dict[str, object]:
    """Collect M7-related log hints without remoteproc lifecycle operations."""

    return _run_board_probe(
        "m7_health",
        _target(host, user, port),
        _m7_health_script(),
        execute=execute,
        timeout_seconds=timeout_seconds,
    )


def ethercat_query_readonly(
    *,
    host: str = DEFAULT_BOARD_HOST,
    user: str = DEFAULT_BOARD_USER,
    port: int = DEFAULT_SSH_PORT,
    execute: bool = False,
    timeout_seconds: int = 8,
) -> dict[str, object]:
    """Run only the existing query path; never start/stop bus or write controls."""

    return _run_board_probe(
        "ethercat_query_readonly",
        _target(host, user, port),
        _ethercat_query_script(),
        execute=execute,
        timeout_seconds=timeout_seconds,
    )


def board_report(
    root: Path,
    *,
    host: str = DEFAULT_BOARD_HOST,
    user: str = DEFAULT_BOARD_USER,
    port: int = DEFAULT_SSH_PORT,
    execute: bool = False,
    timeout_seconds: int = 8,
    output_dir: Path | None = None,
    write_report: bool = True,
) -> dict[str, object]:
    """Build one board-only report from all read-only diagnostic probes."""

    target = _target(host, user, port)
    created_at = datetime.now().astimezone()
    checks = [
        _run_board_probe("board_status", target, _board_status_script(), execute=execute, timeout_seconds=timeout_seconds),
        _run_board_probe("rpmsg_health", target, _rpmsg_health_script(), execute=execute, timeout_seconds=timeout_seconds),
        _run_board_probe("m7_health", target, _m7_health_script(), execute=execute, timeout_seconds=timeout_seconds),
        _run_board_probe(
            "ethercat_query_readonly",
            target,
            _ethercat_query_script(),
            execute=execute,
            timeout_seconds=timeout_seconds,
        ),
    ]
    executed_checks = [item for item in checks if item["executed"]]
    passed = all(item["passed"] is not False for item in checks) if execute else None
    payload: dict[str, object] = {
        "created_at": created_at.isoformat(),
        "mode": "execute" if execute else "dry_run",
        "target": asdict(target),
        "passed": passed,
        "summary": _summarize_board_checks(checks, execute=execute),
        "checks": checks,
        "risk_boundary": _board_boundary(),
        "next_actions": _board_next_actions(checks, execute=execute),
    }
    if write_report:
        report_paths = _write_board_report(root, payload, output_dir=output_dir)
        payload["report"] = report_paths
    return payload


def _run_board_probe(
    name: str,
    target: BoardTarget,
    remote_script: str,
    *,
    execute: bool,
    timeout_seconds: int,
) -> dict[str, object]:
    safety = _validate_readonly_script(remote_script)
    command = _ssh_command(target, remote_script, timeout_seconds=timeout_seconds)
    base_payload: dict[str, object] = {
        "name": name,
        "risk": "board_required" if execute else "offline_ok_dry_run",
        "scope": "read_only_diagnostics",
        "target": asdict(target),
        "execute_requested": execute,
        "executed": False,
        "passed": None,
        "command": command,
        "remote_script": remote_script,
        "safety": safety,
        "returncode": None,
        "stdout": "",
        "stderr": "",
        "diagnosis": {
            "status": "dry_run" if not execute else "pending",
            "reason": "Execution skipped. Re-run with --execute when a board-only read window is available.",
        },
    }
    if not safety["passed"]:
        base_payload["diagnosis"] = {
            "status": "blocked",
            "reason": "Remote script contains blocked control, motion, IO, firmware, or lifecycle token.",
        }
        base_payload["passed"] = False
        return base_payload
    if not execute:
        return base_payload

    try:
        completed = subprocess.run(
            command,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_seconds + 2,
        )
        stdout = completed.stdout[-8000:]
        stderr = completed.stderr[-4000:]
        diagnosis = _diagnose_probe(name, completed.returncode, stdout, stderr)
        base_payload.update(
            {
                "executed": True,
                "passed": diagnosis["passed"],
                "returncode": completed.returncode,
                "stdout": stdout,
                "stderr": stderr,
                "diagnosis": diagnosis,
            }
        )
        return base_payload
    except subprocess.TimeoutExpired as exc:
        base_payload.update(
            {
                "executed": True,
                "passed": False,
                "stdout": exc.stdout[-8000:] if isinstance(exc.stdout, str) else "",
                "stderr": exc.stderr[-4000:] if isinstance(exc.stderr, str) else "",
                "diagnosis": {
                    "status": "timeout",
                    "reason": "SSH command timed out; check board power, network route, and SSH service.",
                    "passed": False,
                },
            }
        )
        return base_payload


def _target(host: str, user: str, port: int) -> BoardTarget:
    return BoardTarget(host=host, user=user, port=port)


def _ssh_command(target: BoardTarget, remote_script: str, *, timeout_seconds: int) -> list[str]:
    destination = f"{target.user}@{target.host}"
    return [
        "ssh",
        "-o",
        "BatchMode=yes",
        "-o",
        "StrictHostKeyChecking=no",
        "-o",
        f"ConnectTimeout={timeout_seconds}",
        "-p",
        str(target.port),
        destination,
        "sh",
        "-lc",
        remote_script,
    ]


def _validate_readonly_script(remote_script: str) -> dict[str, object]:
    lowered = remote_script.lower()
    hits = [token for token in BLOCKED_REMOTE_TOKENS if token in lowered]
    return {
        "passed": not hits,
        "blocked_tokens": hits,
        "policy": "read-only board diagnostics only",
    }


def _board_status_script() -> str:
    return _one_line(
        """
        echo SECTION board_status;
        hostname || true;
        date -Is 2>/dev/null || date || true;
        uname -a || true;
        ip addr show eth0 2>/dev/null | sed -n '1,12p' || true;
        test -d /home && echo CHECK home_dir=present || echo CHECK home_dir=missing;
        test -x /home/a53_send_ec_profile && echo CHECK a53_send_ec_profile=present || echo CHECK a53_send_ec_profile=missing;
        test -f /home/rpmsg_auto_open.py && echo CHECK rpmsg_auto_open=present || echo CHECK rpmsg_auto_open=missing;
        """
    )


def _rpmsg_health_script() -> str:
    return _one_line(
        """
        echo SECTION rpmsg_health;
        ls -l /dev/rpmsg* 2>/dev/null || echo NO_RPMSG_DEVICE;
        python3 /home/rpmsg_auto_open.py --print-dev 2>&1 || echo RPMSG_AUTO_OPEN_FAILED:$?;
        dmesg | grep -i -E 'rpmsg|virtio|remoteproc' | tail -60 || true;
        """
    )


def _m7_health_script() -> str:
    return _one_line(
        """
        echo SECTION m7_health;
        dmesg | grep -i -E 'remoteproc|rpmsg|cm7|m7|ethercat|soem' | tail -120 || true;
        ps | grep -i -E 'rpmsg|ethercat|a53_send' | grep -v grep || true;
        """
    )


def _ethercat_query_script() -> str:
    return _one_line(
        """
        echo SECTION ethercat_query_readonly;
        cd /home || exit 2;
        test -x ./a53_send_ec_profile || { echo A53_SEND_MISSING; exit 3; };
        ./a53_send_ec_profile --query 2>&1;
        """
    )


def _one_line(script: str) -> str:
    return " ".join(line.strip() for line in script.strip().splitlines() if line.strip())


def _diagnose_probe(name: str, returncode: int, stdout: str, stderr: str) -> dict[str, object]:
    combined = f"{stdout}\n{stderr}".lower()
    if returncode != 0 and _looks_like_ssh_failure(combined):
        return {
            "status": "ssh_unreachable",
            "reason": "SSH connection failed; check board power, cable, IP, user, and key/password mode.",
            "passed": False,
        }
    if name == "board_status":
        if returncode != 0:
            return {"status": "board_status_failed", "reason": "Board status command returned non-zero.", "passed": False}
        missing = _missing_checks(stdout)
        if missing:
            return {
                "status": "board_scripts_missing",
                "reason": f"Missing required board file(s): {', '.join(missing)}.",
                "passed": False,
            }
        return {"status": "ok", "reason": "Board is reachable and required read-only helper files are present.", "passed": True}
    if name == "rpmsg_health":
        if "no_rpmsg_device" in combined:
            return {"status": "rpmsg_device_missing", "reason": "No /dev/rpmsg* device was found.", "passed": False}
        if "rpmsg_auto_open_failed" in combined:
            return {"status": "rpmsg_auto_open_failed", "reason": "rpmsg_auto_open.py --print-dev did not complete cleanly.", "passed": False}
        if returncode != 0:
            return {"status": "rpmsg_health_failed", "reason": "RPMsg health command returned non-zero.", "passed": False}
        return {"status": "ok", "reason": "RPMsg device discovery produced readable output.", "passed": True}
    if name == "m7_health":
        if "boot failed" in combined or "bad phdr" in combined:
            return {"status": "m7_remoteproc_error", "reason": "M7/remoteproc logs include boot failure markers.", "passed": False}
        if returncode != 0:
            return {"status": "m7_health_failed", "reason": "M7 health command returned non-zero.", "passed": False}
        if not stdout.strip():
            return {"status": "m7_log_empty", "reason": "No M7-related log lines were returned.", "passed": False}
        return {"status": "ok", "reason": "M7-related logs were collected without boot-failure markers.", "passed": True}
    if name == "ethercat_query_readonly":
        if "a53_send_missing" in combined:
            return {"status": "query_tool_missing", "reason": "/home/a53_send_ec_profile is missing or not executable.", "passed": False}
        if returncode != 0:
            return {"status": "query_failed", "reason": "Read-only profile query returned non-zero.", "passed": False}
        if not stdout.strip():
            return {"status": "query_no_output", "reason": "Read-only profile query returned no output.", "passed": False}
        if _looks_like_profile_query(stdout):
            return {"status": "ok", "reason": "Read-only EtherCAT profile query returned structured status.", "passed": True}
        return {"status": "query_unparsed", "reason": "Query returned output, but no known profile markers were parsed.", "passed": False}
    return {"status": "unknown", "reason": "No diagnosis rule matched.", "passed": returncode == 0}


def _looks_like_ssh_failure(text: str) -> bool:
    tokens = [
        "permission denied",
        "connection timed out",
        "connection refused",
        "no route to host",
        "could not resolve hostname",
        "operation timed out",
    ]
    return any(token in text for token in tokens)


def _missing_checks(stdout: str) -> list[str]:
    missing = []
    for line in stdout.splitlines():
        match = re.match(r"CHECK\s+([A-Za-z0-9_]+)=missing", line.strip())
        if match:
            missing.append(match.group(1))
    return missing


def _looks_like_profile_query(stdout: str) -> bool:
    lowered = stdout.lower()
    markers = ["profile_status", "loaded", "applied", "slaves=", "inop=", "task=", "strategy"]
    return any(marker in lowered for marker in markers)


def _summarize_board_checks(checks: list[dict[str, object]], *, execute: bool) -> str:
    if not execute:
        return "Dry run only. Commands were planned but not executed against the board."
    failed = [item for item in checks if item.get("passed") is False]
    if not failed:
        return "All board-only read diagnostics passed."
    names = ", ".join(str(item.get("name")) for item in failed)
    return f"Board-only diagnostics found issue(s) in: {names}."


def _board_next_actions(checks: list[dict[str, object]], *, execute: bool) -> list[str]:
    if not execute:
        return [
            "Review the planned SSH commands.",
            "Run again with --execute only during a board-only read window.",
            "Do not add start-bus, stop-bus, 0x86, 0x41F1, motion, IO, remoteproc, or firmware actions to this path.",
        ]
    actions = []
    for item in checks:
        diagnosis = item.get("diagnosis", {})
        status = diagnosis.get("status") if isinstance(diagnosis, dict) else ""
        if status == "ssh_unreachable":
            actions.append("Check board power, network cable, IP address, SSH user, and authentication.")
        elif status == "board_scripts_missing":
            actions.append("Upload or verify /home/a53_send_ec_profile and /home/rpmsg_auto_open.py before query testing.")
        elif status == "rpmsg_device_missing":
            actions.append("Confirm M7 firmware is running and Linux exposes /dev/rpmsg* before RPMsg query.")
        elif status == "m7_remoteproc_error":
            actions.append("Keep firmware lifecycle work in a separate hardware window; do not hot reload from this read-only path.")
        elif status in {"query_failed", "query_no_output", "query_unparsed"}:
            actions.append("Inspect RPMsg state and a53_send_ec_profile query output before any bus control.")
    return actions or ["Archive board_report and continue with offline profile/fake harness regression."]


def _board_boundary() -> dict[str, object]:
    return {
        "scope": "board_required_readonly",
        "default_mode": "dry_run",
        "execute_requires": "--execute and a board-only read window",
        "blocked": [
            "start-bus",
            "stop-bus",
            "0x86",
            "0x41F1",
            "robot motion",
            "IO/welding/limit output",
            "remoteproc lifecycle",
            "firmware flash or hot reload",
        ],
    }


def _write_board_report(root: Path, payload: dict[str, object], *, output_dir: Path | None = None) -> dict[str, str]:
    destination = output_dir or (root / "reports" / "board_only")
    destination.mkdir(parents=True, exist_ok=True)
    stem = datetime.now().astimezone().strftime("board_report_%Y%m%d_%H%M%S")
    json_path = destination / f"{stem}.json"
    md_path = destination / f"{stem}.md"
    json_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    md_path.write_text(_render_board_report_markdown(payload), encoding="utf-8")
    return {"json": str(json_path), "markdown": str(md_path)}


def _render_board_report_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# Board-Only Diagnostic Report",
        "",
        f"- created_at: {payload.get('created_at', '')}",
        f"- mode: {payload.get('mode', '')}",
        f"- passed: {payload.get('passed')}",
        f"- summary: {payload.get('summary', '')}",
        "",
        "## Target",
        "",
    ]
    target = payload.get("target", {})
    if isinstance(target, dict):
        lines.extend(
            [
                f"- host: {target.get('host', '')}",
                f"- user: {target.get('user', '')}",
                f"- port: {target.get('port', '')}",
                "",
            ]
        )
    lines.extend(["## Checks", ""])
    for check in payload.get("checks", []):
        if not isinstance(check, dict):
            continue
        diagnosis = check.get("diagnosis", {})
        status = diagnosis.get("status", "") if isinstance(diagnosis, dict) else ""
        reason = diagnosis.get("reason", "") if isinstance(diagnosis, dict) else ""
        lines.append(f"- {check.get('name', '')}: passed={check.get('passed')} status={status} reason={reason}")
    lines.extend(["", "## Next Actions", ""])
    for action in payload.get("next_actions", []):
        lines.append(f"- {action}")
    lines.extend(["", "## Boundary", ""])
    boundary = payload.get("risk_boundary", {})
    if isinstance(boundary, dict):
        lines.append(f"- scope: {boundary.get('scope', '')}")
        lines.append(f"- default_mode: {boundary.get('default_mode', '')}")
        lines.append(f"- execute_requires: {boundary.get('execute_requires', '')}")
        lines.append("- blocked:")
        for item in boundary.get("blocked", []):
            lines.append(f"  - {item}")
    lines.append("")
    return "\n".join(lines)
