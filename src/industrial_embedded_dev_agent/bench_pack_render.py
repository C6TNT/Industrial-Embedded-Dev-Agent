from __future__ import annotations

import json
from pathlib import Path


def render_session_bundle_markdown(
    root: Path,
    session_id: str,
    *,
    output_path: Path | None = None,
) -> dict[str, object]:
    session_dir = root / "reports" / "bench_packs" / "sessions" / session_id
    pack_paths = sorted(session_dir.glob("*.json"))
    if not pack_paths:
        raise FileNotFoundError(f"No bench packs found under session '{session_id}'.")

    packs = [_normalize_render_payload(json.loads(path.read_text(encoding="utf-8"))) for path in pack_paths]
    rendered = _render_session_bundle_template(session_id, packs)
    destination = output_path or _default_session_bundle_path(root, session_id)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(rendered, encoding="utf-8")
    return {
        "session_id": session_id,
        "pack_count": len(pack_paths),
        "input_dir": str(session_dir),
        "output_path": str(destination),
    }


def render_bench_pack_markdown(
    root: Path,
    input_path: Path,
    *,
    template: str,
    output_path: Path | None = None,
) -> dict[str, object]:
    pack = json.loads(input_path.read_text(encoding="utf-8"))
    rendered = _render_pack_template(pack, template=template)
    destination = output_path or _default_rendered_pack_path(root, input_path, template=template)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(rendered, encoding="utf-8")
    return {
        "template": template,
        "input_path": str(input_path),
        "output_path": str(destination),
    }


def _default_rendered_pack_path(root: Path, input_path: Path, *, template: str) -> Path:
    base_name = input_path.stem
    safe_template = template.replace("_", "-")
    return root / "reports" / "bench_packs" / "rendered" / f"{base_name}_{safe_template}.md"


def _default_session_bundle_path(root: Path, session_id: str) -> Path:
    return root / "reports" / "bench_packs" / "rendered" / f"{session_id}_session-bundle.md"


def _render_pack_template(pack: dict[str, object], *, template: str) -> str:
    if template == "first-run":
        return _render_first_run_template(pack)
    if template == "issue":
        return _render_issue_template(pack)
    if template == "session-review":
        return _render_session_review_template(pack)
    raise ValueError(f"Unsupported template: {template}")


def _render_session_bundle_template(session_id: str, packs: list[dict[str, object]]) -> str:
    first = packs[0]
    label = str(first.get("label", ""))
    latest = packs[-1]
    unique_tools = sorted(
        {
            str(pack.get("result", {}).get("plan", {}).get("tool_id", ""))
            for pack in packs
            if pack.get("result", {}).get("plan", {}).get("tool_id")
        }
    )
    unique_modes = sorted({str(pack.get("mode", {}).get("mode", "")) for pack in packs if pack.get("mode", {}).get("mode")})
    confirmed = _build_bundle_confirmed_facts(packs)
    blockers = _build_bundle_blockers(packs)
    next_steps = _build_bundle_next_steps(packs)

    lines = [
        "# Real Bench Session Bundle Review",
        "",
        f"- Session ID: {session_id}",
        f"- Session label: {label}",
        f"- Pack count: {len(packs)}",
        f"- First capture: {first.get('captured_at', '')}",
        f"- Latest capture: {latest.get('captured_at', '')}",
        f"- Execution modes seen: {', '.join(unique_modes) if unique_modes else 'none'}",
        f"- Tools touched: {', '.join(unique_tools) if unique_tools else 'none'}",
        "",
        "## Requests Covered",
        "",
    ]
    for pack in packs:
        lines.append(
            f"- {pack.get('captured_at', '')}: {pack.get('request', '')}"
        )

    lines.extend(
        [
            "",
            "## Session-Level Confirmed Facts",
            "",
            *[f"- {item}" for item in confirmed],
            "",
            "## Session-Level Blockers",
            "",
            *[f"- {item}" for item in blockers],
            "",
            "## Pack Timeline",
            "",
        ]
    )
    for index, pack in enumerate(packs, start=1):
        plan = pack.get("result", {}).get("plan", {})
        execution = pack.get("result", {}).get("execution", {})
        parsed = execution.get("parsed_output", {})
        lines.extend(
            [
                f"### Pack {index}",
                "",
                f"- captured_at: {pack.get('captured_at', '')}",
                f"- request: {pack.get('request', '')}",
                f"- tool_id: {plan.get('tool_id', '')}",
                f"- risk_level: {plan.get('risk_level', '')}",
                f"- returncode: {execution.get('returncode', '')}",
                f"- parsed_status: {parsed.get('status', '')}",
                f"- summary: {parsed.get('summary', execution.get('stderr', ''))}",
                "",
            ]
        )

    lines.extend(
        [
            "## Recommended Next Steps",
            "",
            *[f"- {item}" for item in next_steps],
            "",
            "## Final Session Verdict",
            "",
            f"- {_bundle_verdict(packs)}",
            "",
        ]
    )
    return "\n".join(lines)


def _normalize_render_payload(value):
    if isinstance(value, dict):
        return {key: _normalize_render_payload(item) for key, item in value.items()}
    if isinstance(value, list):
        return [_normalize_render_payload(item) for item in value]
    if isinstance(value, str):
        return _repair_mojibake(value)
    return value


def _repair_mojibake(text: str) -> str:
    suspicious_markers = ("杩", "鍏", "妗", "璇", "鐮", "鏈", "闆", "鍙", "閾", "銆")
    if not any(marker in text for marker in suspicious_markers):
        return text
    for source_encoding in ("gbk", "gb18030"):
        try:
            repaired = text.encode(source_encoding).decode("utf-8")
        except UnicodeError:
            continue
        if repaired != text:
            return repaired
    return text


def _render_first_run_template(pack: dict[str, object]) -> str:
    normalized_pack = _normalize_render_payload(pack)
    captured_at = str(normalized_pack.get("captured_at", ""))
    request = str(normalized_pack.get("request", ""))
    session_id = str(normalized_pack.get("session_id", ""))
    label = str(normalized_pack.get("label", ""))
    mode = normalized_pack.get("mode", {})
    doctor = normalized_pack.get("doctor", {})
    result = normalized_pack.get("result", {})
    plan = result.get("plan", {})
    execution = result.get("execution", {})
    parsed = execution.get("parsed_output", {})
    axes = parsed.get("axes", {})
    axis_health = parsed.get("axis_health", {})
    axis0 = axes.get("0", {})
    axis1 = axes.get("1", {})

    return "\n".join(
        [
            "# Real Bench First-Run Record",
            "",
            f"- Captured at: {captured_at}",
            f"- Session ID: {session_id}",
            f"- Session label: {label}",
            f"- Request: {request}",
            f"- Execution mode: {mode.get('mode', '')}",
            "",
            "## Environment State",
            "",
            f"- `tools mode`: {mode.get('summary', '')}",
            f"- `tools doctor`: execution_mode={doctor.get('execution_mode', '')}, stub_mode_enabled={doctor.get('stub_mode_enabled', '')}, real_mode_ready={doctor.get('real_mode_ready', '')}",
            "",
            "## Plan Result",
            "",
            f"- tool_id: {plan.get('tool_id', '')}",
            f"- risk_level: {plan.get('risk_level', '')}",
            f"- allowed_to_execute: {plan.get('allowed_to_execute', '')}",
            f"- requires_confirmation: {plan.get('requires_confirmation', '')}",
            f"- reason: {plan.get('reason', '')}",
            "",
            "## Execution Result",
            "",
            f"- returncode: {execution.get('returncode', '')}",
            f"- transport_state: {parsed.get('transport_state', '')}",
            f"- poll_count: {parsed.get('poll_count', '')}",
            f"- next_action: {parsed.get('next_action', '')}",
            "",
            "## Axis 0",
            "",
            f"- error_code: {axis0.get('error_code', '')}",
            f"- status_code: {axis0.get('status_code', '')}",
            f"- axis_status: {axis0.get('axis_status', '')}",
            f"- encoder: {axis0.get('encoder', '')}",
            f"- axis_health: {axis_health.get('0', '')}",
            "",
            "## Axis 1",
            "",
            f"- error_code: {axis1.get('error_code', '')}",
            f"- status_code: {axis1.get('status_code', '')}",
            f"- axis_status: {axis1.get('axis_status', '')}",
            f"- encoder: {axis1.get('encoder', '')}",
            f"- axis_health: {axis_health.get('1', '')}",
            "",
            "## Follow-Up",
            "",
            "- Vendor tool comparison:",
            "- SDO / object dictionary comparison:",
            "- Additional notes:",
            "",
        ]
    )


def _render_issue_template(pack: dict[str, object]) -> str:
    normalized_pack = _normalize_render_payload(pack)
    captured_at = str(normalized_pack.get("captured_at", ""))
    request = str(normalized_pack.get("request", ""))
    session_id = str(normalized_pack.get("session_id", ""))
    label = str(normalized_pack.get("label", ""))
    mode = normalized_pack.get("mode", {})
    result = normalized_pack.get("result", {})
    plan = result.get("plan", {})
    execution = result.get("execution", {})
    parsed = execution.get("parsed_output", {})

    return "\n".join(
        [
            "# Real Bench Issue Capture",
            "",
            f"- Captured at: {captured_at}",
            f"- Session ID: {session_id}",
            f"- Session label: {label}",
            f"- Request: {request}",
            f"- Execution mode: {mode.get('mode', '')}",
            "",
            "## Trigger Point",
            "",
            f"- tool_id: {plan.get('tool_id', '')}",
            f"- risk_level: {plan.get('risk_level', '')}",
            f"- allowed_to_execute: {plan.get('allowed_to_execute', '')}",
            f"- returncode: {execution.get('returncode', '')}",
            "",
            "## Observed Symptom",
            "",
            f"- parsed status: {parsed.get('status', '')}",
            f"- summary: {parsed.get('summary', execution.get('stderr', ''))}",
            "",
            "## Raw Evidence",
            "",
            "```json",
            json.dumps(normalized_pack, ensure_ascii=False, indent=2),
            "```",
            "",
            "## Immediate Assessment",
            "",
            "- likely category:",
            "- immediate containment action:",
            "- what was already ruled out:",
            "- what is still unknown:",
            "",
            "## Recommended Next Step",
            "",
            "- ",
            "",
        ]
    )


def _render_session_review_template(pack: dict[str, object]) -> str:
    normalized_pack = _normalize_render_payload(pack)
    captured_at = str(normalized_pack.get("captured_at", ""))
    request = str(normalized_pack.get("request", ""))
    session_id = str(normalized_pack.get("session_id", ""))
    label = str(normalized_pack.get("label", ""))
    mode = normalized_pack.get("mode", {})
    doctor = normalized_pack.get("doctor", {})
    result = normalized_pack.get("result", {})
    plan = result.get("plan", {})
    execution = result.get("execution", {})
    parsed = execution.get("parsed_output", {})
    axes = parsed.get("axes", {})

    confirmed_facts = _build_confirmed_facts(parsed, execution)
    blockers = _build_blockers(doctor, execution, parsed)
    next_step = parsed.get("next_action", "Compare the current evidence pack against vendor-tool and SDO readback.")

    return "\n".join(
        [
            "# Real Bench Session Review",
            "",
            f"- Captured at: {captured_at}",
            f"- Session ID: {session_id}",
            f"- Session label: {label}",
            f"- Request: {request}",
            f"- Execution mode: {mode.get('mode', '')}",
            "",
            "## Session Goal",
            "",
            f"- Planned request: {request}",
            f"- Tool selected: {plan.get('tool_id', '')}",
            f"- Risk level: {plan.get('risk_level', '')}",
            "",
            "## What Was Done",
            "",
            "1. Captured current tool execution mode.",
            "2. Captured WSL environment diagnostics.",
            "3. Generated tool plan for the current request.",
            "4. Captured execution output and parsed structured evidence.",
            "",
            "## Confirmed Facts",
            "",
            *[f"- {item}" for item in confirmed_facts],
            "",
            "## Unconfirmed Assumptions",
            "",
            "- Vendor tool still needs to be compared against the script output.",
            "- SDO / object-dictionary readback still needs to be compared against the script output.",
            "- Bench-side safety state still needs explicit manual confirmation if this is a real run.",
            "",
            "## Key Evidence Produced",
            "",
            f"- returncode: {execution.get('returncode', '')}",
            f"- transport_state: {parsed.get('transport_state', '')}",
            f"- poll_count: {parsed.get('poll_count', '')}",
            f"- axes captured: {', '.join(sorted(axes.keys())) if axes else 'none'}",
            "",
            "## Risk Boundary Status",
            "",
            f"- allowed_to_execute: {plan.get('allowed_to_execute', '')}",
            f"- requires_confirmation: {plan.get('requires_confirmation', '')}",
            f"- should_refuse: {plan.get('should_refuse', '')}",
            "- High-risk L2 actions remain outside this evidence pack.",
            "",
            "## Main Findings",
            "",
            f"1. {parsed.get('summary', execution.get('stderr', 'No summary captured.'))}",
            f"2. Tool mode during capture: {mode.get('mode', '')}",
            f"3. WSL real-mode readiness: {doctor.get('real_mode_ready', '')}",
            "",
            "## Main Blockers",
            "",
            *[f"- {item}" for item in blockers],
            "",
            "## Best Next Step",
            "",
            f"- {next_step}",
            "",
            "## Repository Follow-Up",
            "",
            "- Archive this bench-pack alongside any vendor-tool screenshots.",
            "- If the bench output is meaningful, convert it into a new case or log sample.",
            "- If the output reveals a new pattern, add a benchmark case or parser improvement task.",
            "",
            "## Final Session Verdict",
            "",
            f"- {_session_verdict(plan, execution, parsed)}",
            "",
        ]
    )


def _build_confirmed_facts(parsed: dict[str, object], execution: dict[str, object]) -> list[str]:
    facts: list[str] = []
    if execution.get("returncode") == 0:
        facts.append("The command completed with returncode 0.")
    if parsed.get("transport_state") == "readable":
        facts.append("The transport was readable during capture.")
    poll_count = parsed.get("poll_count")
    if poll_count:
        facts.append(f"The snapshot captured {poll_count} poll cycles.")
    axes = parsed.get("axes", {})
    for axis_id in sorted(axes.keys()):
        payload = axes[axis_id]
        facts.append(
            f"axis{axis_id} returned status_code={payload.get('status_code')} axis_status={payload.get('axis_status')} encoder={payload.get('encoder')}."
        )
    if not facts:
        facts.append("No strong confirmed fact was extracted automatically.")
    return facts


def _build_blockers(doctor: dict[str, object], execution: dict[str, object], parsed: dict[str, object]) -> list[str]:
    blockers: list[str] = []
    if doctor.get("execution_mode") == "real_unavailable":
        blockers.append("Real WSL runtime path is not ready yet.")
    if execution.get("returncode") not in (0, None):
        blockers.append("Execution did not finish cleanly.")
    if parsed.get("status") == "environment_error":
        blockers.append("Environment error is blocking bench validation.")
    if not blockers:
        blockers.append("No hard blocker was extracted automatically; vendor / SDO comparison is still pending.")
    return blockers


def _session_verdict(plan: dict[str, object], execution: dict[str, object], parsed: dict[str, object]) -> str:
    if plan.get("should_refuse"):
        return "unsafe to continue without manual review"
    if parsed.get("status") == "environment_error":
        return "environment partially recovered"
    if execution.get("returncode") == 0 and parsed.get("transport_state") == "readable":
        return "read-only validation succeeded"
    if execution.get("returncode") == 0:
        return "mismatch found, needs follow-up"
    return "environment partially recovered"


def _build_bundle_confirmed_facts(packs: list[dict[str, object]]) -> list[str]:
    facts: list[str] = []
    readable_count = 0
    success_count = 0
    axis_lines: list[str] = []
    for pack in packs:
        execution = pack.get("result", {}).get("execution", {})
        parsed = execution.get("parsed_output", {})
        if execution.get("returncode") == 0:
            success_count += 1
        if parsed.get("transport_state") == "readable":
            readable_count += 1
        axes = parsed.get("axes", {})
        for axis_id in sorted(axes.keys()):
            payload = axes[axis_id]
            axis_lines.append(
                f"axis{axis_id} status_code={payload.get('status_code')} axis_status={payload.get('axis_status')} encoder={payload.get('encoder')}"
            )
    if success_count:
        facts.append(f"{success_count} pack(s) completed with returncode 0.")
    if readable_count:
        facts.append(f"{readable_count} pack(s) reported readable transport.")
    if axis_lines:
        facts.append("Observed axis snapshots: " + "; ".join(axis_lines[:4]))
    if not facts:
        facts.append("No strong session-level fact was extracted automatically.")
    return facts


def _build_bundle_blockers(packs: list[dict[str, object]]) -> list[str]:
    blockers: list[str] = []
    if any(pack.get("doctor", {}).get("execution_mode") == "real_unavailable" for pack in packs):
        blockers.append("At least one pack still saw the real WSL runtime as unavailable.")
    if any(pack.get("result", {}).get("execution", {}).get("parsed_output", {}).get("status") == "environment_error" for pack in packs):
        blockers.append("At least one pack hit an environment-level error.")
    if any(
        pack.get("result", {}).get("plan", {}).get("should_refuse")
        for pack in packs
    ):
        blockers.append("This session includes at least one request outside the allowed execution boundary.")
    if not blockers:
        blockers.append("No hard blocker was extracted automatically; vendor / SDO comparison is still pending.")
    return blockers


def _build_bundle_next_steps(packs: list[dict[str, object]]) -> list[str]:
    steps: list[str] = []
    for pack in reversed(packs):
        parsed = pack.get("result", {}).get("execution", {}).get("parsed_output", {})
        next_action = str(parsed.get("next_action", "")).strip()
        if next_action and next_action not in steps:
            steps.append(next_action)
    if not steps:
        steps.append("Compare the captured packs against vendor-tool and SDO readback.")
    return steps[:3]


def _bundle_verdict(packs: list[dict[str, object]]) -> str:
    if any(pack.get("result", {}).get("plan", {}).get("should_refuse") for pack in packs):
        return "session includes blocked high-risk requests"
    if any(pack.get("result", {}).get("execution", {}).get("parsed_output", {}).get("status") == "environment_error" for pack in packs):
        return "environment still needs cleanup before reliable bench validation"
    if any(pack.get("result", {}).get("execution", {}).get("parsed_output", {}).get("transport_state") == "readable" for pack in packs):
        return "read-only session evidence captured successfully"
    return "session evidence captured, but follow-up comparison is still required"
