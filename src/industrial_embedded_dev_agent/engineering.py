from __future__ import annotations

import json
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from .analysis import analyze_text
from .benchmarks import load_benchmark_items, summarize_benchmark
from .chunking import load_chunk_documents, summarize_chunks


TEXT_EXTENSIONS = {
    ".bat",
    ".c",
    ".cfg",
    ".cpp",
    ".csv",
    ".h",
    ".hpp",
    ".ini",
    ".json",
    ".jsonl",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".toml",
    ".tsv",
    ".txt",
    ".xml",
    ".yaml",
    ".yml",
}

SKIP_DIRS = {".git", "__pycache__", ".pytest_cache", ".mypy_cache", ".ruff_cache", ".venv", "venv"}

SENSITIVE_NAME_PATTERNS = [
    re.compile(r"(^|/|\\)\.env($|\.)", re.IGNORECASE),
    re.compile(r"(^|/|\\)id_(rsa|dsa|ed25519)($|\.)", re.IGNORECASE),
    re.compile(r"\.(pem|p12|pfx|key|crt|cer)$", re.IGNORECASE),
    re.compile(r"(secret|credential|credentials|token|passwd|password)", re.IGNORECASE),
]

SECRET_PATTERNS = [
    ("private_key_block", re.compile(r"-----BEGIN [A-Z ]*PRIVATE KEY-----")),
    ("github_token", re.compile(r"\b(?:ghp|gho|ghu|ghs|ghr)_[A-Za-z0-9_]{20,}\b")),
    ("github_pat", re.compile(r"\bgithub_pat_[A-Za-z0-9_]{20,}\b")),
    ("openai_key", re.compile(r"\bsk-[A-Za-z0-9]{20,}\b")),
    ("aws_access_key", re.compile(r"\bAKIA[0-9A-Z]{16}\b")),
    ("slack_token", re.compile(r"\bxox[baprs]-[A-Za-z0-9-]{20,}\b")),
    (
        "generic_assignment",
        re.compile(
            r"(?i)\b(password|passwd|pwd|secret|token|api[_-]?key|access[_-]?key)\b\s*[:=]\s*[\"']?([^\"'\s]{8,})"
        ),
    ),
]

HARDWARE_KEYWORDS = {
    "board-status": "board_required",
    "board status": "board_required",
    "board-report": "board_required",
    "board report": "board_required",
    "rpmsg-health": "board_required",
    "m7-health": "board_required",
    "ethercat-query-readonly": "board_required",
    "ssh": "board_required",
    "scp": "board_required",
    "reboot": "firmware_required",
    "hot reload": "firmware_required",
    "热重载": "firmware_required",
    "0x86": "control_word_required",
    "0x41f1": "output_gate_required",
    "motion": "robot_motion_required",
    "move": "robot_motion_required",
    "运动": "robot_motion_required",
    "使能": "control_word_required",
    "io output": "io_required",
    "写输出": "io_required",
    "焊接": "io_required",
    "限位": "io_required",
    "remoteproc": "firmware_required",
    "firmware": "firmware_required",
    "flash": "firmware_required",
    "start-bus": "board_required",
    "stop-bus": "board_required",
}


def scan_secrets(root: Path, *, include_history: bool = False) -> dict[str, object]:
    """Scan current repository files, and optionally git history, for obvious secret patterns."""

    paths = _repo_paths(root)
    name_hits: list[str] = []
    content_hits: list[dict[str, object]] = []
    scanned = 0
    skipped = 0

    for rel in paths:
        rel_norm = rel.replace("\\", "/")
        if _should_skip_path(rel_norm):
            continue
        if any(pattern.search(rel_norm) for pattern in SENSITIVE_NAME_PATTERNS):
            name_hits.append(rel_norm)

        path = root / rel
        if not path.is_file():
            continue
        if path.suffix.lower() not in TEXT_EXTENSIONS and path.name != ".gitignore":
            skipped += 1
            continue
        text = _read_text_or_none(path)
        if text is None:
            skipped += 1
            continue
        scanned += 1
        content_hits.extend(_scan_text_for_secrets(rel_norm, text))

    history_hits = _scan_history_for_secrets(root) if include_history else []
    passed = not name_hits and not content_hits and not history_hits
    return {
        "passed": passed,
        "include_history": include_history,
        "paths_considered": len(paths),
        "text_files_scanned": scanned,
        "binary_or_nontext_skipped": skipped,
        "sensitive_filename_hits": name_hits,
        "content_secret_hits": content_hits,
        "history_secret_hits": history_hits,
    }


def run_pre_push_check(
    root: Path,
    *,
    include_history: bool = False,
    run_checks: bool = True,
    include_offline: bool = False,
    include_rag: bool = False,
    rag_item_type: str | None = "tool_safety",
) -> dict[str, object]:
    """Run the local gate we expect before pushing to GitHub."""

    from .runner import run_local_checks_with_options

    checks: list[dict[str, object]] = []
    secret_result = scan_secrets(root, include_history=include_history)
    checks.append({"name": "secret_scan", "passed": secret_result["passed"], "details": secret_result})

    diff_check = _run_command(root, ["git", "diff", "--check"])
    checks.append(
        {
            "name": "git_diff_check",
            "passed": diff_check["returncode"] == 0,
            "details": diff_check,
        }
    )

    if run_checks:
        local_checks = run_local_checks_with_options(
            root,
            include_offline=include_offline,
            include_rag=include_rag,
            rag_item_type=rag_item_type,
        )
        checks.append({"name": "local_regression", "passed": local_checks["passed"], "details": local_checks})

    return {
        "passed": all(item["passed"] for item in checks),
        "checks": checks,
        "next_action": "safe_to_push" if all(item["passed"] for item in checks) else "fix_failed_checks_before_push",
    }


def project_status(root: Path, *, run_checks: bool = False) -> dict[str, object]:
    """Summarize baseline, data shape, git state, and optional local check health."""

    from .material_workspace import build_material_status
    from .runner import run_local_checks_with_options
    from .tools import current_project_baseline

    benchmark_path = root / "data" / "benchmark" / "benchmark_v1.jsonl"
    chunks_path = root / "data" / "chunks" / "doc_chunks_v1.jsonl"
    benchmark_items = load_benchmark_items(benchmark_path) if benchmark_path.exists() else []
    chunks = load_chunk_documents(chunks_path) if chunks_path.exists() else []
    status = {
        "baseline": current_project_baseline(root),
        "benchmark": summarize_benchmark(benchmark_items),
        "chunks": summarize_chunks(chunks),
        "material_workspace": build_material_status(root),
        "git": _git_summary(root),
        "hardware_boundary": {
            "offline_ok": [
                "documentation",
                "RAG and benchmark tuning",
                "secret scan",
                "fake harness/replay summary",
                "report import draft generation",
                "board-only diagnostic dry-run planning",
            ],
            "requires_hardware_window": [
                "board-only read diagnostics with --execute",
                "real RPMsg/EtherCAT query/start/stop",
                "0x86 control word",
                "0x41F1 gate unlock",
                "robot motion",
                "IO/welding/limit output",
                "firmware deployment or remoteproc lifecycle validation",
            ],
        },
    }
    if run_checks:
        status["local_checks"] = run_local_checks_with_options(
            root,
            include_offline=True,
            include_rag=True,
            rag_item_type="tool_safety",
        )
    return status


def gsd_status(root: Path) -> dict[str, object]:
    """Report whether the repository is ready for guarded GSD automation."""

    planning_root = root / ".planning"
    required_files = [
        "PROJECT.md",
        "REQUIREMENTS.md",
        "ROADMAP.md",
        "STATE.md",
        "GSD_BOUNDARY.md",
    ]
    file_status = {name: (planning_root / name).exists() for name in required_files}
    config_path = planning_root / "config.json"
    config: dict[str, object] = {}
    if config_path.exists():
        try:
            config = json.loads(config_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            config = {"error": "invalid_json"}
    sdk_status = _detect_gsd_sdk(root)
    return {
        "planning_root": str(planning_root),
        "planning_ready": planning_root.exists() and all(file_status.values()),
        "required_files": file_status,
        "config": config,
        "sdk": sdk_status,
        "automation_scope": {
            "allowed": [
                "offline code edits",
                "pytest and benchmark regression",
                "raw project search and inventory generation",
                "curated memory draft generation",
                "RAG/chunk/material updates",
                "fake harness and replay summaries",
                "board-only diagnostic dry-runs",
                "secret-scan and pre-push-check",
                "documentation and review pack generation",
            ],
            "blocked": [
                "ssh/scp/reboot",
                "start-bus/stop-bus on real hardware",
                "0x86 control word",
                "0x41F1 unlock",
                "robot motion",
                "IO/welding/limit output",
                "remoteproc lifecycle, firmware flashing, or M7 hot reload",
            ],
        },
        "next_safe_command": "spindle check --include-offline --include-rag --rag-type tool_safety --no-write-summary",
    }


def run_gsd_offline(
    root: Path,
    *,
    include_history: bool = False,
    write_report: bool = True,
    update_state: bool = True,
) -> dict[str, object]:
    """Run the local GSD-safe autonomous gate without touching real hardware."""

    started_at = datetime.now().astimezone()
    status = gsd_status(root)
    local_gate = _run_guarded_local_check(root)
    pre_push = run_pre_push_check(
        root,
        include_history=include_history,
        run_checks=False,
    )
    phase_summary = _summarize_gsd_phase_files(root)
    passed = bool(status["planning_ready"]) and bool(local_gate.get("passed")) and bool(pre_push.get("passed"))
    payload: dict[str, object] = {
        "created_at": started_at.isoformat(),
        "mode": "gsd_offline_autonomous",
        "passed": passed,
        "planning_ready": status["planning_ready"],
        "sdk": status["sdk"],
        "automation_scope": status["automation_scope"],
        "phase_summary": phase_summary,
        "checks": [
            {
                "name": "gsd_status",
                "passed": bool(status["planning_ready"]),
                "details": {
                    "required_files": status["required_files"],
                    "config_safety": (status.get("config") or {}).get("safety", {}),
                },
            },
            {
                "name": "guarded_local_gate",
                "passed": bool(local_gate.get("passed")),
                "details": local_gate,
            },
            {
                "name": "pre_push_static_gate",
                "passed": bool(pre_push.get("passed")),
                "details": pre_push,
            },
        ],
        "blocked_autonomous_scope": _blocked_autonomous_scope_from_status(status),
        "next_action": "ready_for_review_or_push" if passed else "fix_failed_gsd_gate",
    }

    if write_report:
        report_paths = _write_gsd_offline_report(root, payload)
        payload["report"] = report_paths
    if update_state:
        payload["state_update"] = _append_gsd_state_note(root, payload)
    return payload


def draft_project_fact(
    root: Path,
    fact_text: str,
    *,
    title: str | None = None,
    source: str | None = None,
    category: str | None = None,
    output_dir: Path | None = None,
) -> dict[str, object]:
    """Create a reviewable draft for one new project fact without touching canonical data."""

    timestamp = datetime.now().astimezone()
    slug = _slugify(title or fact_text[:48]) or "project-fact"
    destination = output_dir or (root / "reports" / "project_fact_drafts" / f"{timestamp.strftime('%Y%m%d_%H%M%S')}_{slug}")
    destination.mkdir(parents=True, exist_ok=True)
    diagnosis = analyze_text(fact_text, mode="auto")
    detected_category = category or diagnosis.issue_category
    suggested_updates = _suggest_fact_updates(detected_category, fact_text)
    benchmark_candidate = _build_fact_benchmark_candidate(detected_category, fact_text, title)

    payload = {
        "created_at": timestamp.isoformat(),
        "title": title or slug.replace("-", " "),
        "source": source or "",
        "category": detected_category,
        "fact_text": fact_text,
        "diagnosis": {
            "summary": diagnosis.summary,
            "issue_category": diagnosis.issue_category,
            "cause_labels": diagnosis.cause_labels,
            "action_labels": diagnosis.action_labels,
            "risk_level": diagnosis.risk_level,
            "should_refuse": diagnosis.should_refuse,
            "evidence": diagnosis.evidence,
        },
        "suggested_updates": suggested_updates,
        "benchmark_candidate": benchmark_candidate,
    }

    fact_json = destination / "fact_draft.json"
    fact_md = destination / "fact_draft.md"
    benchmark_json = destination / "benchmark_candidate.json"
    checklist_md = destination / "update_checklist.md"
    fact_json.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    benchmark_json.write_text(json.dumps(benchmark_candidate, ensure_ascii=False, indent=2), encoding="utf-8")
    fact_md.write_text(_render_fact_draft_markdown(payload), encoding="utf-8")
    checklist_md.write_text(_render_fact_checklist_markdown(payload), encoding="utf-8")

    return {
        "output_dir": str(destination),
        "fact_draft_json": str(fact_json),
        "fact_draft_markdown": str(fact_md),
        "benchmark_candidate": str(benchmark_json),
        "update_checklist": str(checklist_md),
        "suggested_updates": suggested_updates,
    }


def draft_material_fact(
    root: Path,
    fact_text: str,
    *,
    source_path: str,
    title: str | None = None,
    material_root: Path | None = None,
    output_dir: Path | None = None,
) -> dict[str, object]:
    """Create a source-backed memory draft from the material workspace."""

    from .material_workspace import resolve_material_workspace

    workspace = resolve_material_workspace(root, material_root)
    normalized_source = source_path.replace("\\", "/")
    source_full = workspace / normalized_source
    source_exists = source_full.exists()
    result = draft_project_fact(
        root,
        fact_text,
        title=title,
        source=f"{workspace.name}/{normalized_source}",
        category=None,
        output_dir=output_dir,
    )
    source_check = {
        "material_root": str(workspace),
        "source_path": normalized_source,
        "source_full_path": str(source_full),
        "source_exists": source_exists,
        "memory_policy": "review before promoting to canonical memory",
    }
    source_check_path = Path(result["output_dir"]) / "source_check.json"
    source_check_path.write_text(json.dumps(source_check, ensure_ascii=False, indent=2), encoding="utf-8")
    result["source_check"] = str(source_check_path)
    result["source_exists"] = source_exists
    return result


def import_real_report(root: Path, report_path: Path, *, output_dir: Path | None = None) -> dict[str, object]:
    """Convert a real JSON report into reviewable LOG/replay drafts."""

    payload = json.loads(report_path.read_text(encoding="utf-8"))
    timestamp = datetime.now().astimezone()
    destination = output_dir or (
        root / "reports" / "report_imports" / f"{timestamp.strftime('%Y%m%d_%H%M%S')}_{_slugify(report_path.stem)}"
    )
    destination.mkdir(parents=True, exist_ok=True)
    extracted = _extract_report_fields(payload)
    replay_scenario = _build_replay_scenario(extracted, report_path)
    log_entry = _build_log_entry_draft(extracted, report_path)

    summary = {
        "source_report": str(report_path),
        "created_at": timestamp.isoformat(),
        "extracted": extracted,
        "log_entry_draft": log_entry,
        "replay_scenario_draft": replay_scenario,
        "suggested_updates": [
            "review log_entry_draft.md and append a curated LOG-* row to data/materials/material_index_v1.md",
            "review replay_scenario_draft.json and place accepted cases under the fake harness replay fixtures",
            "add or update benchmark items when this report changes expected Agent behavior",
            "run chunks build and local checks after canonical updates",
        ],
    }

    summary_json = destination / "report_import_summary.json"
    log_md = destination / "log_entry_draft.md"
    replay_json = destination / "replay_scenario_draft.json"
    summary_json.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    log_md.write_text(_render_log_entry_markdown(log_entry, extracted), encoding="utf-8")
    replay_json.write_text(json.dumps(replay_scenario, ensure_ascii=False, indent=2), encoding="utf-8")

    return {
        "output_dir": str(destination),
        "summary_json": str(summary_json),
        "log_entry_draft": str(log_md),
        "replay_scenario_draft": str(replay_json),
        "extracted": extracted,
    }


def summarize_fake_regression(root: Path, input_path: Path, *, output_dir: Path | None = None) -> dict[str, object]:
    """Parse fake harness regression JSON files and generate a reviewable summary."""

    timestamp = datetime.now().astimezone()
    destination = output_dir or (root / "reports" / "fake_regression_summaries" / timestamp.strftime("%Y%m%d_%H%M%S"))
    destination.mkdir(parents=True, exist_ok=True)
    records = _load_regression_records(input_path)
    passed = sum(1 for item in records if item.get("passed") is True)
    failed = sum(1 for item in records if item.get("passed") is False)
    summary = {
        "created_at": timestamp.isoformat(),
        "source": str(input_path),
        "total_records": len(records),
        "passed": passed,
        "failed": failed,
        "pass_rate": round(passed / len(records), 4) if records else 0.0,
        "records": records,
        "suggested_updates": [
            "append accepted fake regression result to LOG-* material entries",
            "add replay or XML batch cases when a new scenario appears",
            "refresh benchmark items if a failure mode becomes a formal expectation",
        ],
    }
    summary_json = destination / "fake_regression_summary.json"
    summary_md = destination / "fake_regression_summary.md"
    material_draft = destination / "material_entry_draft.md"
    summary_json.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    summary_md.write_text(_render_fake_regression_summary(summary), encoding="utf-8")
    material_draft.write_text(_render_fake_material_entry(summary), encoding="utf-8")
    return {
        "output_dir": str(destination),
        "summary_json": str(summary_json),
        "summary_markdown": str(summary_md),
        "material_entry_draft": str(material_draft),
        "total_records": len(records),
        "passed": passed,
        "failed": failed,
        "pass_rate": summary["pass_rate"],
    }


def audit_hardware_action(root: Path, request: str) -> dict[str, object]:
    """Explain whether a request crosses hardware, robot, IO, or firmware boundaries."""

    lowered = request.lower()
    requirements = sorted({scope for token, scope in HARDWARE_KEYWORDS.items() if token in lowered or token in request})
    if _is_board_diagnostic_dry_run(lowered):
        requirements = ["offline_ok"]
    diagnosis_mode = "safety" if requirements else "auto"
    diagnosis = analyze_text(request, mode=diagnosis_mode)
    if not requirements and diagnosis.risk_level in {"L0_readonly", "L1_low_risk_exec"}:
        requirements = ["offline_ok"]
    boundary = "offline_ok" if requirements == ["offline_ok"] else "hardware_window_required"
    required_conditions = []
    if "board_required" in requirements:
        required_conditions.extend(
            [
                "board powered and network reachable",
                "manual permission for board-only read window",
                "no start-bus/stop-bus/control-word/IO/firmware action in the same command",
            ]
        )
    if "control_word_required" in requirements:
        required_conditions.extend(["operator at emergency stop", "fault state understood", "0x86 permission confirmed"])
    if "output_gate_required" in requirements:
        required_conditions.extend(["0x41F1 unlock explicitly approved", "dynamic write path reviewed"])
    if "robot_motion_required" in requirements:
        required_conditions.extend(["site cleared", "speed/position limits confirmed", "operator at emergency stop"])
    if "io_required" in requirements:
        required_conditions.extend(["IO wiring and load state confirmed", "welding/limit outputs reviewed"])
    if "firmware_required" in requirements:
        required_conditions.extend(["rollback image available", "boot path confirmed", "hardware window reserved"])
    return {
        "request": request,
        "boundary": boundary,
        "requirements": requirements,
        "should_execute_automatically": boundary == "offline_ok" and not diagnosis.should_refuse,
        "requires_manual_confirmation": boundary != "offline_ok" or diagnosis.should_refuse,
        "diagnosis": {
            "summary": diagnosis.summary,
            "issue_category": diagnosis.issue_category,
            "risk_level": diagnosis.risk_level,
            "should_refuse": diagnosis.should_refuse,
            "evidence": diagnosis.evidence,
        },
        "required_conditions": sorted(set(required_conditions)),
        "safe_alternative": "Use readonly profile/query, fake harness replay, or report import drafts before any hardware action.",
    }


def _is_board_diagnostic_dry_run(lowered_request: str) -> bool:
    board_diag_tokens = [
        "board-status",
        "board status",
        "board-report",
        "board report",
        "rpmsg-health",
        "m7-health",
        "ethercat-query-readonly",
    ]
    if not any(token in lowered_request for token in board_diag_tokens):
        return False
    execute_tokens = ["--execute", " execute", "真实执行", "上板执行", "ssh ", "start-bus", "stop-bus"]
    return not any(token in lowered_request for token in execute_tokens)


def write_regression_overview(root: Path, check_payload: dict[str, object], *, output_dir: Path | None = None) -> dict[str, object]:
    timestamp = datetime.now().astimezone()
    destination = output_dir or (root / "reports" / "regression_overviews")
    destination.mkdir(parents=True, exist_ok=True)
    stem = timestamp.strftime("%Y%m%d_%H%M%S")
    overview_json = destination / f"regression_overview_{stem}.json"
    overview_md = destination / f"regression_overview_{stem}.md"
    payload = {
        "created_at": timestamp.isoformat(),
        "passed": bool(check_payload.get("passed")),
        "checks": check_payload.get("checks", []),
        "next_action": "ready_for_review_or_push" if check_payload.get("passed") else "inspect_failed_checks",
    }
    overview_json.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    overview_md.write_text(_render_regression_overview(payload), encoding="utf-8")
    return {
        "regression_overview_json": str(overview_json),
        "regression_overview_markdown": str(overview_md),
    }


def _run_guarded_local_check(root: Path) -> dict[str, object]:
    from .runner import run_local_checks_with_options

    return run_local_checks_with_options(
        root,
        include_offline=True,
        include_rag=True,
        rag_item_type="tool_safety",
    )


def _summarize_gsd_phase_files(root: Path) -> dict[str, object]:
    phases_root = root / ".planning" / "phases"
    phase_items: list[dict[str, object]] = []
    if phases_root.exists():
        for plan_path in sorted(phases_root.glob("*/PLAN.md")):
            text = _read_text_or_none(plan_path) or ""
            checks = [line.strip() for line in text.splitlines() if line.strip().startswith("- [")]
            done = [line for line in checks if line.startswith("- [x]") or line.startswith("- [X]")]
            phase_items.append(
                {
                    "phase": plan_path.parent.name,
                    "plan": str(plan_path),
                    "checklist_total": len(checks),
                    "checklist_done": len(done),
                    "has_plan": True,
                }
            )
    return {
        "phase_count": len(phase_items),
        "phases": phase_items,
    }


def _blocked_autonomous_scope_from_status(status: dict[str, object]) -> list[str]:
    automation_scope = status.get("automation_scope")
    if isinstance(automation_scope, dict):
        blocked = automation_scope.get("blocked", [])
        if isinstance(blocked, list):
            return [str(item) for item in blocked]
    return []


def _write_gsd_offline_report(root: Path, payload: dict[str, object]) -> dict[str, str]:
    destination = root / "reports" / "gsd_runs"
    destination.mkdir(parents=True, exist_ok=True)
    stem = datetime.now().astimezone().strftime("gsd_offline_run_%Y%m%d_%H%M%S")
    report_json = destination / f"{stem}.json"
    report_md = destination / f"{stem}.md"
    report_json.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    report_md.write_text(_render_gsd_offline_report(payload), encoding="utf-8")
    return {
        "json": str(report_json),
        "markdown": str(report_md),
    }


def _append_gsd_state_note(root: Path, payload: dict[str, object]) -> dict[str, object]:
    state_path = root / ".planning" / "STATE.md"
    if not state_path.exists():
        return {"updated": False, "reason": "missing_state_file", "path": str(state_path)}
    summary = (
        "\n## Last Offline GSD Run\n\n"
        f"- created_at: {payload['created_at']}\n"
        f"- passed: {payload['passed']}\n"
        f"- next_action: {payload['next_action']}\n"
        f"- guarded_local_gate: {_check_status(payload, 'guarded_local_gate')}\n"
        f"- pre_push_static_gate: {_check_status(payload, 'pre_push_static_gate')}\n"
        "- hardware_scope: board diagnostics dry-run only; --execute requires a board-only read window; blocked for start-bus/stop-bus/0x86/0x41F1/robot motion/IO/remoteproc lifecycle/firmware\n"
    )
    original = state_path.read_text(encoding="utf-8")
    marker = "\n## Last Offline GSD Run\n\n"
    if marker in original:
        original = original.split(marker, 1)[0].rstrip() + "\n"
    state_path.write_text(original.rstrip() + summary, encoding="utf-8")
    return {"updated": True, "path": str(state_path)}


def _check_status(payload: dict[str, object], name: str) -> str:
    for check in payload.get("checks", []):
        if isinstance(check, dict) and check.get("name") == name:
            return "PASS" if check.get("passed") else "FAIL"
    return "UNKNOWN"


def _render_gsd_offline_report(payload: dict[str, object]) -> str:
    lines = [
        "# GSD Offline Run Report",
        "",
        f"- created_at: {payload['created_at']}",
        f"- mode: {payload['mode']}",
        f"- passed: {payload['passed']}",
        f"- next_action: {payload['next_action']}",
        "",
        "## Checks",
        "",
    ]
    for check in payload.get("checks", []):
        lines.append(f"- {check.get('name')}: {'PASS' if check.get('passed') else 'FAIL'}")
    lines.extend(
        [
            "",
            "## Autonomous Boundary",
            "",
            "The offline runner may edit code, run tests, rebuild chunks, run benchmarks, scan secrets, and generate reports.",
            "It must not execute real board, bus, robot, IO, firmware, or M7 lifecycle actions.",
            "",
            "Blocked autonomous scopes:",
            "",
        ]
    )
    for item in payload.get("blocked_autonomous_scope", []):
        lines.append(f"- {item}")
    lines.extend(["", "## Phase Summary", ""])
    phase_summary = payload.get("phase_summary", {})
    if isinstance(phase_summary, dict):
        for phase in phase_summary.get("phases", []):
            if isinstance(phase, dict):
                lines.append(
                    f"- {phase.get('phase')}: checklist {phase.get('checklist_done')}/{phase.get('checklist_total')}"
                )
    return "\n".join(lines) + "\n"


def _repo_paths(root: Path) -> list[str]:
    try:
        tracked = _git_output(root, ["git", "ls-files"]).splitlines()
        untracked = _git_output(root, ["git", "ls-files", "--others", "--exclude-standard"]).splitlines()
        paths = sorted(set(tracked + untracked))
        if paths:
            return paths
    except RuntimeError:
        pass
    return sorted(str(path.relative_to(root)) for path in root.rglob("*") if path.is_file())


def _should_skip_path(path: str) -> bool:
    return bool(set(path.split("/")) & SKIP_DIRS)


def _read_text_or_none(path: Path) -> str | None:
    for encoding in ("utf-8", "utf-8-sig", "gbk"):
        try:
            return path.read_text(encoding=encoding)
        except UnicodeDecodeError:
            continue
        except OSError:
            return None
    return None


def _scan_text_for_secrets(rel: str, text: str) -> list[dict[str, object]]:
    hits: list[dict[str, object]] = []
    for lineno, line in enumerate(text.splitlines(), 1):
        for label, pattern in SECRET_PATTERNS:
            if not pattern.search(line):
                continue
            hits.append(
                {
                    "file": rel,
                    "line": lineno,
                    "label": label,
                    "preview": _redact_secret_preview(line.strip()),
                }
            )
    return hits


def _scan_history_for_secrets(root: Path) -> list[dict[str, object]]:
    try:
        revs = _git_output(root, ["git", "rev-list", "--all"]).splitlines()
    except RuntimeError:
        return []
    hits: list[dict[str, object]] = []
    for label, pattern in SECRET_PATTERNS:
        for rev in revs:
            completed = subprocess.run(
                ["git", "grep", "-I", "-n", "-E", pattern.pattern, rev],
                cwd=str(root),
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if completed.returncode not in (0, 1):
                continue
            for line in completed.stdout.splitlines():
                hits.append({"label": label, "preview": _redact_secret_preview(line)})
    return hits


def _redact_secret_preview(text: str) -> str:
    compact = text[:180] + ("..." if len(text) > 180 else "")
    return re.sub(r"([A-Za-z0-9_\-]{6})[A-Za-z0-9_\-]{8,}([A-Za-z0-9_\-]{4})", r"\1***\2", compact)


def _run_command(root: Path, command: list[str]) -> dict[str, object]:
    try:
        completed = subprocess.run(command, cwd=str(root), capture_output=True, text=True, encoding="utf-8", errors="replace")
    except FileNotFoundError as exc:
        return {
            "command": command,
            "returncode": 127,
            "stdout": "",
            "stderr": str(exc),
        }
    return {
        "command": command,
        "returncode": completed.returncode,
        "stdout": completed.stdout[-4000:],
        "stderr": completed.stderr[-4000:],
    }


def _detect_gsd_sdk(root: Path) -> dict[str, object]:
    candidates = [
        ["gsd-sdk", "--version"],
        [str(Path.home() / "bin" / "gsd-sdk.cmd"), "--version"],
    ]
    for command in candidates:
        completed = _run_command(root, command)
        if completed["returncode"] == 0:
            return {
                "available": True,
                "command": command[0],
                "version": str(completed.get("stdout", "")).strip(),
            }
    return {
        "available": False,
        "command": "",
        "version": "",
        "note": "Install or log in to GSD before running gsd-sdk auto.",
    }


def _git_output(root: Path, command: list[str]) -> str:
    completed = subprocess.run(command, cwd=str(root), capture_output=True, text=True, encoding="utf-8", errors="replace")
    if completed.returncode != 0:
        raise RuntimeError(completed.stderr.strip())
    return completed.stdout


def _git_summary(root: Path) -> dict[str, object]:
    return {
        "branch": _safe_git(root, ["git", "branch", "--show-current"]).strip(),
        "head": _safe_git(root, ["git", "log", "-1", "--oneline"]).strip(),
        "status_short": _safe_git(root, ["git", "status", "--short"]).splitlines(),
        "ahead_behind_origin_main": _safe_git(root, ["git", "rev-list", "--left-right", "--count", "origin/main...HEAD"]).strip(),
    }


def _safe_git(root: Path, command: list[str]) -> str:
    try:
        return _git_output(root, command)
    except RuntimeError:
        return ""


def _slugify(value: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9\u4e00-\u9fff]+", "-", value.strip()).strip("-").lower()
    return slug[:64]


def _suggest_fact_updates(category: str, fact_text: str) -> list[str]:
    updates = [
        "data/materials/material_index_v1.md",
        "data/materials/current_ethercat_dynamic_profile_project_v1.md",
        "data/chunks/doc_chunks_v1.jsonl via chunks build",
    ]
    if category in {"safety", "verification_tooling", "ethercat_profile", "pdo_layout", "runtime_takeover"}:
        updates.append("data/benchmark/benchmark_v1.jsonl")
    if any(token in fact_text.lower() for token in ["0x86", "0x41f1", "motion", "remoteproc", "io output"]):
        updates.append("data/benchmark/tool_safety_v1.jsonl")
    return updates


def _build_fact_benchmark_candidate(category: str, fact_text: str, title: str | None) -> dict[str, object]:
    item_id = f"draft-{datetime.now().strftime('%Y%m%d%H%M%S')}"
    if category == "safety":
        return {
            "id": item_id,
            "item_type": "tool_safety",
            "input": {"message": fact_text},
            "expected": {
                "should_refuse_auto_execute": True,
                "risk_level": "L2_high_risk_exec",
                "must_include": ["人工确认"],
            },
            "tags": ["draft", "safety"],
            "difficulty": "draft",
        }
    if "log" in category or "profile" in fact_text.lower() or "report" in fact_text.lower():
        diagnosis = analyze_text(fact_text, mode="log")
        return {
            "id": item_id,
            "item_type": "log_attribution",
            "input": {"log_text": fact_text},
            "expected": {
                "fault_type": diagnosis.issue_category,
                "must_include_causes": diagnosis.cause_labels,
                "must_include_actions": diagnosis.action_labels,
            },
            "tags": ["draft", diagnosis.issue_category],
            "difficulty": "draft",
        }
    return {
        "id": item_id,
        "item_type": "knowledge_qa",
        "input": {"question": title or "这条项目事实说明了什么？"},
        "expected": {"must_include": _important_terms(fact_text), "preferred_sources": ["CUR-001"]},
        "tags": ["draft", category],
        "difficulty": "draft",
    }


def _important_terms(text: str) -> list[str]:
    terms = []
    for token in ["0x4100", "0x41F1", "axis1", "12/28", "19/13", "fake harness", "remoteproc", "IO"]:
        if token.lower() in text.lower():
            terms.append(token)
    return terms or [text[:24]]


def _render_fact_draft_markdown(payload: dict[str, object]) -> str:
    diagnosis = payload["diagnosis"]
    lines = [
        "# Project Fact Draft",
        "",
        f"- title: {payload['title']}",
        f"- source: {payload['source']}",
        f"- category: {payload['category']}",
        f"- created_at: {payload['created_at']}",
        "",
        "## Fact",
        "",
        str(payload["fact_text"]),
        "",
        "## Diagnosis",
        "",
        f"- summary: {diagnosis['summary']}",
        f"- issue_category: {diagnosis['issue_category']}",
        f"- cause_labels: {', '.join(diagnosis['cause_labels'])}",
        f"- action_labels: {', '.join(diagnosis['action_labels'])}",
        f"- risk_level: {diagnosis['risk_level']}",
        "",
        "## Suggested Updates",
        "",
    ]
    lines.extend(f"- {item}" for item in payload["suggested_updates"])
    return "\n".join(lines) + "\n"


def _render_fact_checklist_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# Project Fact Update Checklist",
        "",
        "- [ ] Review fact wording and remove any sensitive site details.",
        "- [ ] Decide whether it becomes CUR, LOG, CASE, or benchmark material.",
        "- [ ] Update canonical material only after review.",
        "- [ ] Rebuild chunks.",
        "- [ ] Run local checks.",
        "",
        "## Candidate Files To Update",
        "",
    ]
    lines.extend(f"- [ ] {item}" for item in payload["suggested_updates"])
    return "\n".join(lines) + "\n"


def _extract_report_fields(payload: Any) -> dict[str, object]:
    keys = [
        "driver",
        "strategy",
        "axis",
        "logical_axis",
        "slave",
        "slave_id",
        "ob",
        "ib",
        "status_word",
        "status",
        "error_code",
        "actual_position_before",
        "actual_position_after",
        "actual_position",
        "actual_vel",
        "pass",
        "passed",
        "fail_reason",
    ]
    extracted: dict[str, object] = {}
    for key in keys:
        value = _find_key(payload, key)
        if value is not None:
            extracted[key] = value
    if "passed" not in extracted and "pass" in extracted:
        extracted["passed"] = extracted["pass"]
    if "fault_mode" not in extracted:
        extracted["fault_mode"] = "none" if _truthy(extracted.get("passed", False)) else "imported_failure"
    return extracted


def _find_key(payload: Any, key: str) -> Any:
    if isinstance(payload, dict):
        for raw_key, value in payload.items():
            if str(raw_key).lower() == key.lower():
                return value
        for value in payload.values():
            found = _find_key(value, key)
            if found is not None:
                return found
    elif isinstance(payload, list):
        for item in payload:
            found = _find_key(item, key)
            if found is not None:
                return found
    return None


def _truthy(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.lower() in {"true", "pass", "passed", "ok", "1"}
    return bool(value)


def _build_replay_scenario(extracted: dict[str, object], report_path: Path) -> dict[str, object]:
    return {
        "source_report": str(report_path),
        "driver": extracted.get("driver", "unknown"),
        "axis": extracted.get("axis", extracted.get("logical_axis")),
        "slave": extracted.get("slave", extracted.get("slave_id")),
        "ob": extracted.get("ob"),
        "ib": extracted.get("ib"),
        "strategy": extracted.get("strategy"),
        "status_word": extracted.get("status_word", extracted.get("status")),
        "error_code": extracted.get("error_code", 0),
        "actual_position_before": extracted.get("actual_position_before"),
        "actual_position_after": extracted.get("actual_position_after", extracted.get("actual_position")),
        "actual_vel": extracted.get("actual_vel"),
        "fault_mode": extracted.get("fault_mode", "none"),
    }


def _build_log_entry_draft(extracted: dict[str, object], report_path: Path) -> dict[str, object]:
    return {
        "source": str(report_path),
        "scenario": extracted.get("driver", "unknown"),
        "summary": (
            f"Imported report axis={extracted.get('axis', extracted.get('logical_axis'))} "
            f"slave={extracted.get('slave', extracted.get('slave_id'))} "
            f"ob={extracted.get('ob')} ib={extracted.get('ib')} "
            f"passed={extracted.get('passed')}"
        ),
        "suggested_tag": "verification_tooling" if _truthy(extracted.get("passed", False)) else "imported_failure",
    }


def _render_log_entry_markdown(log_entry: dict[str, object], extracted: dict[str, object]) -> str:
    return (
        "# LOG Entry Draft\n\n"
        f"- source: `{log_entry['source']}`\n"
        f"- scenario: {log_entry['scenario']}\n"
        f"- summary: {log_entry['summary']}\n"
        f"- suggested_tag: `{log_entry['suggested_tag']}`\n\n"
        "## Extracted Fields\n\n"
        + "\n".join(f"- {key}: {value}" for key, value in sorted(extracted.items()))
        + "\n"
    )


def _load_regression_records(input_path: Path) -> list[dict[str, object]]:
    paths = sorted(input_path.glob("*.json")) if input_path.is_dir() else [input_path]
    records: list[dict[str, object]] = []
    for path in paths:
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        _collect_regression_records(payload, records, source=str(path))
    return records


def _collect_regression_records(payload: Any, records: list[dict[str, object]], *, source: str, prefix: str = "") -> None:
    if isinstance(payload, dict):
        has_pass = "passed" in payload or "pass" in payload
        if has_pass:
            name = str(payload.get("scenario") or payload.get("name") or payload.get("id") or prefix or source)
            records.append(
                {
                    "source": source,
                    "name": name,
                    "passed": _truthy(payload.get("passed", payload.get("pass"))),
                    "fail_reason": payload.get("fail_reason", payload.get("reason", "")),
                }
            )
        for key, value in payload.items():
            _collect_regression_records(value, records, source=source, prefix=f"{prefix}.{key}" if prefix else str(key))
    elif isinstance(payload, list):
        for index, value in enumerate(payload):
            _collect_regression_records(value, records, source=source, prefix=f"{prefix}[{index}]")


def _render_fake_regression_summary(summary: dict[str, object]) -> str:
    lines = [
        "# Fake Harness Regression Summary",
        "",
        f"- source: `{summary['source']}`",
        f"- total_records: {summary['total_records']}",
        f"- passed: {summary['passed']}",
        f"- failed: {summary['failed']}",
        f"- pass_rate: {summary['pass_rate']}",
        "",
        "## Failed Records",
        "",
    ]
    failed = [item for item in summary["records"] if item.get("passed") is False]
    if failed:
        lines.extend(f"- {item['name']}: {item.get('fail_reason', '')}" for item in failed)
    else:
        lines.append("- none")
    return "\n".join(lines) + "\n"


def _render_fake_material_entry(summary: dict[str, object]) -> str:
    return (
        "# Material Entry Draft\n\n"
        f"- 场景: fake harness regression\n"
        f"- 摘要: {summary['total_records']} records, {summary['passed']} passed, {summary['failed']} failed\n"
        f"- 建议标签: `verification_tooling`\n"
        f"- 来源: `{summary['source']}`\n"
    )


def _render_regression_overview(payload: dict[str, object]) -> str:
    lines = [
        "# Regression Overview",
        "",
        f"- created_at: {payload['created_at']}",
        f"- passed: {payload['passed']}",
        f"- next_action: {payload['next_action']}",
        "",
        "## Checks",
        "",
    ]
    for check in payload.get("checks", []):
        lines.append(f"- {check.get('name')}: {'PASS' if check.get('passed') else 'FAIL'}")
    failed = [check for check in payload.get("checks", []) if not check.get("passed")]
    lines.extend(["", "## Risk Items", ""])
    if failed:
        lines.extend(f"- inspect `{check.get('name')}` details" for check in failed)
    else:
        lines.append("- none")
    return "\n".join(lines) + "\n"
