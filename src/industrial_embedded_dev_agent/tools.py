from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from dataclasses import asdict
from datetime import datetime
from pathlib import Path
from uuid import uuid4

from .analysis import analyze_text
from .models import ToolExecutionResult, ToolPlan, ToolSpec


SAFE_EXECUTION_RISKS = {"L0_readonly", "L1_low_risk_exec"}
WSL_STUB_LIBRARY = "/home/librobot.so.1.0.0"
WSL_STUB_FLAG = ".ieda_wsl_stub_enabled"
WSL_STUB_PROFILE = ".ieda_stub_profile.json"
DEFAULT_STUB_SCENARIO = "nominal"
STUB_SCENARIOS = {
    "nominal": "Readable transport with stable idle snapshots on both axes.",
    "encoder_stall": "Transport stays readable, but encoder feedback stops changing on both axes.",
    "axis1_fault": "Transport stays readable, while axis1 reports a non-zero error code and abnormal status.",
    "open_rpmsg_fail": "Transport initialization degrades immediately and OpenRpmsg returns a failure code.",
}

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


def list_stub_scenarios() -> list[dict[str, str]]:
    return [{"scenario": name, "description": description} for name, description in STUB_SCENARIOS.items()]


def inspect_wsl_environment(root: Path) -> dict[str, object]:
    stub_mode_enabled = (root / WSL_STUB_FLAG).exists()
    stub_library_present = _command_success(["wsl.exe", "bash", "-lc", f"test -f {WSL_STUB_LIBRARY}"])
    stub_scenario = _read_stub_scenario(root)
    return {
        "wsl_available": _command_success(["wsl.exe", "bash", "-lc", "true"]),
        "python3_available": _command_success(["wsl.exe", "bash", "-lc", "command -v python3 >/dev/null 2>&1"]),
        "gcc_available": _command_success(["wsl.exe", "bash", "-lc", "command -v gcc >/dev/null 2>&1"]),
        "execution_mode": _resolve_execution_mode(root, stub_library_present=stub_library_present),
        "stub_mode_enabled": stub_mode_enabled,
        "stub_scenario": stub_scenario,
        "stub_scenario_description": STUB_SCENARIOS.get(stub_scenario, ""),
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


def setup_wsl_stub_environment(root: Path, *, scenario: str = DEFAULT_STUB_SCENARIO) -> dict[str, object]:
    resolved_scenario = _normalize_stub_scenario(scenario)
    script_path = root / "scripts" / "setup_wsl_stub.ps1"
    command = [
        "powershell.exe",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(script_path),
        "-Scenario",
        resolved_scenario,
    ]
    completed = subprocess.run(command, cwd=str(root), capture_output=True, text=True)
    return {
        "command": command,
        "scenario": resolved_scenario,
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


def prepare_real_bench_package(
    root: Path,
    *,
    session_id: str | None = None,
    label: str | None = None,
    output_dir: Path | None = None,
) -> dict[str, object]:
    timestamp = datetime.now().astimezone()
    resolved_session_id = _resolve_session_id(session_id, label, timestamp) or f"real-bench-{timestamp.strftime('%Y%m%d')}"
    destination = output_dir or (root / "reports" / "real_bench_prep" / resolved_session_id)
    destination.mkdir(parents=True, exist_ok=True)
    doctor = inspect_wsl_environment(root)
    git_context = _collect_git_context(root)

    templates = {
        "00_index.md": _render_real_bench_index(
            resolved_session_id,
            label,
            timestamp,
            doctor=doctor,
            git_context=git_context,
        ),
        "01_readiness_checklist.md": _wrap_real_bench_doc(
            title="Real Bench Readiness Checklist",
            session_id=resolved_session_id,
            label=label,
            timestamp=timestamp,
            source_path=root / "docs" / "real_bench_readiness_checklist.md",
        ),
        "02_first_run_record.md": _wrap_real_bench_doc(
            title="Real Bench First-Run Record",
            session_id=resolved_session_id,
            label=label,
            timestamp=timestamp,
            source_path=root / "docs" / "real_bench_first_run_template.md",
        ),
        "03_issue_capture.md": _wrap_real_bench_doc(
            title="Real Bench Issue Capture",
            session_id=resolved_session_id,
            label=label,
            timestamp=timestamp,
            source_path=root / "docs" / "real_bench_issue_capture_template.md",
        ),
        "04_session_review.md": _wrap_real_bench_doc(
            title="Real Bench Session Review",
            session_id=resolved_session_id,
            label=label,
            timestamp=timestamp,
            source_path=root / "docs" / "real_bench_session_review_template.md",
        ),
    }

    written_files = []
    doctor_snapshot_path = destination / "doctor_snapshot.json"
    doctor_snapshot_payload = {
        "session_id": resolved_session_id,
        "label": label,
        "generated_at": timestamp.isoformat(),
        "git_context": git_context,
        "doctor": doctor,
    }
    doctor_snapshot_path.write_text(json.dumps(doctor_snapshot_payload, ensure_ascii=False, indent=2), encoding="utf-8")
    written_files.append(str(doctor_snapshot_path))

    seed_request = "先读一下 axis0/axis1 的状态字、错误码和编码器，我要看当前链路是不是还活着。"
    plan_seed_path = destination / "plan_seed.json"
    plan_seed_payload = {
        "session_id": resolved_session_id,
        "label": label,
        "generated_at": timestamp.isoformat(),
        "request": seed_request,
        "expected_tool_id": "SCRIPT-004",
        "expected_risk_level": "L0_readonly",
        "expected_allowed_to_execute": True,
        "commands": {
            "plan": f'python -m industrial_embedded_dev_agent tools plan "{seed_request}"',
            "run": f'python -m industrial_embedded_dev_agent tools run "{seed_request}" --execute',
            "bench_pack": (
                f'python -m industrial_embedded_dev_agent tools bench-pack "{seed_request}" '
                f'--session-id {resolved_session_id}'
            ),
        },
    }
    plan_seed_path.write_text(json.dumps(plan_seed_payload, ensure_ascii=False, indent=2), encoding="utf-8")
    written_files.append(str(plan_seed_path))

    for file_name, content in templates.items():
        file_path = destination / file_name
        file_path.write_text(content, encoding="utf-8")
        written_files.append(str(file_path))

    return {
        "session_id": resolved_session_id,
        "label": label,
        "output_dir": str(destination),
        "doctor": doctor,
        "git_context": git_context,
        "doctor_snapshot_path": str(doctor_snapshot_path),
        "plan_seed_path": str(plan_seed_path),
        "files": written_files,
    }


def kickoff_real_bench(
    root: Path,
    seed_path: Path,
    *,
    execute: bool = False,
    render_first_run: bool = False,
    render_session_review: bool = False,
    timeout_seconds: int = 20,
) -> dict[str, object]:
    seed = json.loads(seed_path.read_text(encoding="utf-8"))
    request = str(seed["request"])
    session_id = str(seed["session_id"])
    label = seed.get("label")

    bench_pack = build_bench_pack(
        root,
        request,
        session_id=session_id,
        label=label if isinstance(label, str) else None,
        execute=execute,
        timeout_seconds=timeout_seconds,
    )
    payload = {
        "seed_path": str(seed_path),
        "request": request,
        "session_id": session_id,
        "label": label,
        "execute": execute,
        "bench_pack": bench_pack,
    }
    if render_first_run or render_session_review:
        from .bench_pack_render import render_bench_pack_markdown as render_bench_pack_draft

        bench_pack_path = Path(str(bench_pack["saved_to"]))
        if render_first_run:
            payload["rendered_first_run"] = render_bench_pack_draft(
                root,
                bench_pack_path,
                template="first-run",
            )
        if render_session_review:
            payload["rendered_session_review"] = render_bench_pack_draft(
                root,
                bench_pack_path,
                template="session-review",
            )
    archived = _archive_kickoff_outputs(seed_path, payload)
    if archived is not None:
        payload["archive"] = archived
    return payload


def finish_real_bench(
    root: Path,
    *,
    session_id: str,
    prep_dir: Path | None = None,
) -> dict[str, object]:
    resolved_prep_dir = prep_dir or (root / "reports" / "real_bench_prep" / session_id)
    finish_dir = resolved_prep_dir / "finish_outputs"
    finish_dir.mkdir(parents=True, exist_ok=True)

    artifacts: list[str] = []
    session_bundle_path: Path | None = None
    compare_path: Path | None = None

    session_dir = root / "reports" / "bench_packs" / "sessions" / session_id
    pack_paths = sorted(session_dir.glob("*.json")) if session_dir.exists() else []
    if pack_paths:
        from .bench_pack_render import render_session_bundle_markdown, compare_latest_bench_packs_in_session

        session_bundle = render_session_bundle_markdown(root, session_id, output_path=finish_dir / "session_bundle.md")
        session_bundle_path = Path(str(session_bundle["output_path"]))
        artifacts.append(str(session_bundle_path))

        if len(pack_paths) >= 2:
            compare_summary = compare_latest_bench_packs_in_session(
                root,
                session_id,
                output_path=finish_dir / "latest_compare.md",
            )
            compare_path = Path(str(compare_summary["output_path"]))
            artifacts.append(str(compare_path))

    kickoff_dir = resolved_prep_dir / "kickoff_outputs"
    kickoff_summary_json = kickoff_dir / "run_summary.json"
    kickoff_summary_md = kickoff_dir / "run_summary.md"
    kickoff_summary_payload: dict[str, object] = {}
    if kickoff_summary_json.exists():
        kickoff_summary_payload = json.loads(kickoff_summary_json.read_text(encoding="utf-8"))
        copied_json = finish_dir / "kickoff_run_summary.json"
        shutil.copyfile(kickoff_summary_json, copied_json)
        artifacts.append(str(copied_json))
    if kickoff_summary_md.exists():
        copied_md = finish_dir / "kickoff_run_summary.md"
        shutil.copyfile(kickoff_summary_md, copied_md)
        artifacts.append(str(copied_md))

    summary_payload = {
        "session_id": session_id,
        "prep_dir": str(resolved_prep_dir),
        "pack_count": len(pack_paths),
        "kickoff_outputs_present": kickoff_dir.exists(),
        "session_bundle_path": str(session_bundle_path) if session_bundle_path else "",
        "latest_compare_path": str(compare_path) if compare_path else "",
        "kickoff_summary": kickoff_summary_payload,
    }
    summary_json_path = finish_dir / "final_summary.json"
    summary_json_path.write_text(json.dumps(summary_payload, ensure_ascii=False, indent=2), encoding="utf-8")
    artifacts.append(str(summary_json_path))

    summary_md_path = finish_dir / "final_summary.md"
    summary_md_path.write_text(_render_finish_summary_markdown(summary_payload), encoding="utf-8")
    artifacts.append(str(summary_md_path))

    candidate_exports = _export_finish_candidates(
        finish_dir,
        session_id=session_id,
        summary_payload=summary_payload,
    )
    artifacts.extend(candidate_exports["files"])

    return {
        "session_id": session_id,
        "prep_dir": str(resolved_prep_dir),
        "output_dir": str(finish_dir),
        "files": artifacts,
        "final_summary_json": str(summary_json_path),
        "final_summary_markdown": str(summary_md_path),
        "candidate_exports": candidate_exports,
    }


def review_finish_candidates(
    root: Path,
    *,
    session_id: str,
    prep_dir: Path | None = None,
) -> dict[str, object]:
    resolved_prep_dir = prep_dir or (root / "reports" / "real_bench_prep" / session_id)
    finish_dir = resolved_prep_dir / "finish_outputs"
    candidate_dir = finish_dir / "candidate_exports"
    review_dir = finish_dir / "candidate_review"
    review_dir.mkdir(parents=True, exist_ok=True)

    case_path = candidate_dir / "case_candidate.md"
    log_path = candidate_dir / "log_candidate.json"
    benchmark_path = candidate_dir / "benchmark_candidate.json"
    quality_json_path = finish_dir / "candidate_quality_check" / "candidate_quality_check.json"

    case_text = case_path.read_text(encoding="utf-8") if case_path.exists() else ""
    log_payload = json.loads(log_path.read_text(encoding="utf-8")) if log_path.exists() else {}
    benchmark_payload = json.loads(benchmark_path.read_text(encoding="utf-8")) if benchmark_path.exists() else {}
    quality_payload = json.loads(quality_json_path.read_text(encoding="utf-8")) if quality_json_path.exists() else {}

    review_payload = {
        "session_id": session_id,
        "candidate_dir": str(candidate_dir),
        "has_case_candidate": case_path.exists(),
        "has_log_candidate": log_path.exists(),
        "has_benchmark_candidate": benchmark_path.exists(),
        "suggested_tag": log_payload.get("suggested_tag", ""),
        "tool_id": log_payload.get("tool_id", ""),
        "risk_level": log_payload.get("risk_level", ""),
        "benchmark_item_type": benchmark_payload.get("item_type", ""),
        "benchmark_tags": benchmark_payload.get("tags", []),
        "quality_check_present": quality_json_path.exists(),
        "quality_overall_passed": quality_payload.get("overall_passed", False),
        "quality_level": quality_payload.get("quality_level", ""),
        "quality_score": quality_payload.get("quality_score", 0),
        "quality_warnings": quality_payload.get("warnings", []),
        "quality_warning_categories": quality_payload.get("warning_categories", []),
        "quality_guidance": _guidance_for_warning_categories(quality_payload.get("warning_categories", [])),
        "quality_summary_path": str(quality_json_path) if quality_json_path.exists() else "",
    }
    review_payload["review_recommendation"] = _review_recommendation(review_payload)

    review_json_path = review_dir / "review_summary.json"
    review_json_path.write_text(json.dumps(review_payload, ensure_ascii=False, indent=2), encoding="utf-8")

    review_md_path = review_dir / "review_summary.md"
    review_md_path.write_text(
        _render_finish_candidate_review_markdown(
            review_payload,
            case_preview=case_text,
            benchmark_payload=benchmark_payload,
            quality_payload=quality_payload,
        ),
        encoding="utf-8",
    )

    return {
        "session_id": session_id,
        "candidate_dir": str(candidate_dir),
        "output_dir": str(review_dir),
        "review_summary_json": str(review_json_path),
        "review_summary_markdown": str(review_md_path),
    }


def candidate_quality_check(
    root: Path,
    *,
    session_id: str,
    prep_dir: Path | None = None,
) -> dict[str, object]:
    resolved_prep_dir = prep_dir or (root / "reports" / "real_bench_prep" / session_id)
    finish_dir = resolved_prep_dir / "finish_outputs"
    candidate_dir = finish_dir / "candidate_exports"
    quality_dir = finish_dir / "candidate_quality_check"
    quality_dir.mkdir(parents=True, exist_ok=True)

    case_path = candidate_dir / "case_candidate.md"
    log_path = candidate_dir / "log_candidate.json"
    benchmark_path = candidate_dir / "benchmark_candidate.json"

    case_text = case_path.read_text(encoding="utf-8") if case_path.exists() else ""
    log_payload = json.loads(log_path.read_text(encoding="utf-8")) if log_path.exists() else {}
    benchmark_payload = json.loads(benchmark_path.read_text(encoding="utf-8")) if benchmark_path.exists() else {}

    case_result = _evaluate_case_candidate(case_path, case_text)
    log_result = _evaluate_log_candidate(log_path, log_payload)
    benchmark_result = _evaluate_benchmark_candidate(benchmark_path, benchmark_payload)

    warning_details = [
        *case_result["warning_details"],
        *log_result["warning_details"],
        *benchmark_result["warning_details"],
    ]
    warnings = [detail["message"] for detail in warning_details]
    warning_categories = _collect_warning_categories([case_result, log_result, benchmark_result])
    quality_level = _overall_quality_level([case_result, log_result, benchmark_result])
    quality_score = _overall_quality_score([case_result, log_result, benchmark_result])
    overall_passed = bool(case_result["passed"] and log_result["passed"] and benchmark_result["passed"])
    next_action = (
        "Candidate set looks good enough for manual review. Continue with review-finish-candidates."
        if quality_level == "good"
        else (
            "Review the warnings first, then edit weak candidate drafts before promotion."
            if quality_level == "weak"
            else "Candidate set is blocked. Fix missing or critical fields before review and promotion."
        )
    )

    payload = {
        "session_id": session_id,
        "candidate_dir": str(candidate_dir),
        "overall_passed": overall_passed,
        "quality_level": quality_level,
        "quality_score": quality_score,
        "case_candidate": case_result,
        "log_candidate": log_result,
        "benchmark_candidate": benchmark_result,
        "warnings": warnings,
        "warning_details": warning_details,
        "warning_categories": warning_categories,
        "next_action": next_action,
    }

    quality_json_path = quality_dir / "candidate_quality_check.json"
    quality_json_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    quality_md_path = quality_dir / "candidate_quality_check.md"
    quality_md_path.write_text(_render_candidate_quality_check_markdown(payload), encoding="utf-8")

    return {
        "session_id": session_id,
        "candidate_dir": str(candidate_dir),
        "output_dir": str(quality_dir),
        "candidate_quality_check_json": str(quality_json_path),
        "candidate_quality_check_markdown": str(quality_md_path),
        "overall_passed": overall_passed,
    }


def promote_finish_candidates(
    root: Path,
    *,
    session_id: str,
    prep_dir: Path | None = None,
) -> dict[str, object]:
    resolved_prep_dir = prep_dir or (root / "reports" / "real_bench_prep" / session_id)
    finish_dir = resolved_prep_dir / "finish_outputs"
    candidate_dir = finish_dir / "candidate_exports"
    review_summary_path = finish_dir / "candidate_review" / "review_summary.json"

    case_path = candidate_dir / "case_candidate.md"
    log_path = candidate_dir / "log_candidate.json"
    benchmark_path = candidate_dir / "benchmark_candidate.json"
    review_payload = json.loads(review_summary_path.read_text(encoding="utf-8")) if review_summary_path.exists() else {}
    review_recommendation = str(review_payload.get("review_recommendation", "")).strip()
    soft_blocked = review_recommendation in {"edit_before_promote", "hold_for_manual_analysis"}
    next_step = "continue_to_pending_merge"
    if review_recommendation == "hold_for_manual_analysis":
        next_step = "stop_and_analyze"
    elif review_recommendation == "edit_before_promote":
        next_step = "run_manual_edit"
    promotion_warning = ""
    if review_recommendation == "hold_for_manual_analysis":
        promotion_warning = (
            "Promotion proceeded even though review_recommendation=hold_for_manual_analysis. "
            "Manual analysis is strongly recommended before any formal merge."
        )
    elif review_recommendation == "edit_before_promote":
        promotion_warning = (
            "Promotion proceeded even though review_recommendation=edit_before_promote. "
            "Refine the candidate drafts before formal merge."
        )

    pending_root = root / "data" / "pending"
    pending_cases_dir = pending_root / "cases"
    pending_logs_dir = pending_root / "logs"
    pending_benchmarks_dir = pending_root / "benchmarks"
    pending_root.mkdir(parents=True, exist_ok=True)
    pending_cases_dir.mkdir(parents=True, exist_ok=True)
    pending_logs_dir.mkdir(parents=True, exist_ok=True)
    pending_benchmarks_dir.mkdir(parents=True, exist_ok=True)

    copied_files: list[str] = []
    promoted_case_path = pending_cases_dir / f"{session_id}_case_candidate.md"
    promoted_log_path = pending_logs_dir / f"{session_id}_log_candidate.json"
    promoted_benchmark_path = pending_benchmarks_dir / f"{session_id}_benchmark_candidate.json"
    promotion_record_path = pending_root / "promotion_records" / f"{session_id}_promotion_record.json"
    promotion_markdown_path = pending_root / "promotion_records" / f"{session_id}_promotion_record.md"
    promotion_record_path.parent.mkdir(parents=True, exist_ok=True)

    if case_path.exists():
        shutil.copyfile(case_path, promoted_case_path)
        copied_files.append(str(promoted_case_path))
    if log_path.exists():
        shutil.copyfile(log_path, promoted_log_path)
        copied_files.append(str(promoted_log_path))
    benchmark_payload: dict[str, object] = {}
    if benchmark_path.exists():
        shutil.copyfile(benchmark_path, promoted_benchmark_path)
        copied_files.append(str(promoted_benchmark_path))
        benchmark_payload = json.loads(benchmark_path.read_text(encoding="utf-8"))

    pending_jsonl_path = pending_benchmarks_dir / "pending_benchmark_candidates.jsonl"
    if benchmark_payload:
        with pending_jsonl_path.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(benchmark_payload, ensure_ascii=False) + "\n")
        copied_files.append(str(pending_jsonl_path))

    promotion_record = {
        "session_id": session_id,
        "prep_dir": str(resolved_prep_dir),
        "candidate_dir": str(candidate_dir),
        "review_summary_path": str(review_summary_path) if review_summary_path.exists() else "",
        "review_recommendation": review_recommendation,
        "quality_warning_categories": review_payload.get("quality_warning_categories", []),
        "promotion_guidance": _guidance_for_warning_categories(review_payload.get("quality_warning_categories", [])),
        "soft_blocked": soft_blocked,
        "next_step": next_step,
        "promotion_warning": promotion_warning,
        "promoted_case_path": str(promoted_case_path) if case_path.exists() else "",
        "promoted_log_path": str(promoted_log_path) if log_path.exists() else "",
        "promoted_benchmark_path": str(promoted_benchmark_path) if benchmark_path.exists() else "",
        "pending_benchmark_jsonl": str(pending_jsonl_path) if benchmark_payload else "",
    }
    promotion_record_path.write_text(json.dumps(promotion_record, ensure_ascii=False, indent=2), encoding="utf-8")
    copied_files.append(str(promotion_record_path))
    promotion_markdown_path.write_text(_render_promotion_record_markdown(promotion_record), encoding="utf-8")
    copied_files.append(str(promotion_markdown_path))

    return {
        "session_id": session_id,
        "pending_root": str(pending_root),
        "files": copied_files,
        "promotion_record": str(promotion_record_path),
        "promotion_record_markdown": str(promotion_markdown_path),
        "review_recommendation": review_recommendation,
        "soft_blocked": soft_blocked,
        "next_step": next_step,
        "promotion_warning": promotion_warning,
    }


def plan_pending_merge(root: Path) -> dict[str, object]:
    pending_root = root / "data" / "pending"
    cases_dir = pending_root / "cases"
    logs_dir = pending_root / "logs"
    benchmarks_dir = pending_root / "benchmarks"
    promotion_records_dir = pending_root / "promotion_records"
    plan_dir = pending_root / "merge_plan"
    plan_dir.mkdir(parents=True, exist_ok=True)

    case_files = sorted(cases_dir.glob("*.md")) if cases_dir.exists() else []
    log_files = sorted(logs_dir.glob("*.json")) if logs_dir.exists() else []
    benchmark_files = sorted(
        [path for path in benchmarks_dir.glob("*.json") if path.name != "pending_benchmark_candidates.jsonl"]
    ) if benchmarks_dir.exists() else []
    promotion_records = _load_promotion_records(promotion_records_dir)

    case_candidates: list[dict[str, object]] = []
    log_candidates: list[dict[str, object]] = []
    benchmark_candidates: list[dict[str, object]] = []
    deferred_candidates: list[dict[str, object]] = []

    for path in case_files:
        session_id = path.name.removesuffix("_case_candidate.md")
        review_info = promotion_records.get(session_id, {})
        next_step = str(review_info.get("next_step", "continue_to_pending_merge"))
        candidate = {
            "session_id": session_id,
            "source": str(path),
            "suggested_target": "data/materials/",
            "action": "review_and_copy",
            "review_recommendation": review_info.get("review_recommendation", ""),
            "quality_level": review_info.get("quality_level", ""),
            "quality_score": review_info.get("quality_score", 0),
            "quality_warning_categories": review_info.get("quality_warning_categories", []),
            "promotion_guidance": review_info.get("promotion_guidance", []),
            "next_step": next_step,
        }
        if next_step == "continue_to_pending_merge":
            case_candidates.append(candidate)
        else:
            deferred_candidates.append(
                {
                    **candidate,
                    "candidate_type": "case",
                    "deferred_reason": f"Deferred because next_step={next_step}.",
                }
            )

    for path in log_files:
        session_id = path.name.removesuffix("_log_candidate.json")
        review_info = promotion_records.get(session_id, {})
        next_step = str(review_info.get("next_step", "continue_to_pending_merge"))
        candidate = {
            "session_id": session_id,
            "source": str(path),
            "suggested_target": "data/materials/ or future log corpus",
            "action": "review_and_reclassify",
            "review_recommendation": review_info.get("review_recommendation", ""),
            "quality_level": review_info.get("quality_level", ""),
            "quality_score": review_info.get("quality_score", 0),
            "quality_warning_categories": review_info.get("quality_warning_categories", []),
            "promotion_guidance": review_info.get("promotion_guidance", []),
            "next_step": next_step,
        }
        if next_step == "continue_to_pending_merge":
            log_candidates.append(candidate)
        else:
            deferred_candidates.append(
                {
                    **candidate,
                    "candidate_type": "log",
                    "deferred_reason": f"Deferred because next_step={next_step}.",
                }
            )

    for path in benchmark_files:
        session_id = path.name.removesuffix("_benchmark_candidate.json")
        review_info = promotion_records.get(session_id, {})
        next_step = str(review_info.get("next_step", "continue_to_pending_merge"))
        candidate = {
            "session_id": session_id,
            "source": str(path),
            "suggested_target": "data/benchmark/benchmark_v1.jsonl",
            "action": "review_then_append",
            "review_recommendation": review_info.get("review_recommendation", ""),
            "quality_level": review_info.get("quality_level", ""),
            "quality_score": review_info.get("quality_score", 0),
            "quality_warning_categories": review_info.get("quality_warning_categories", []),
            "promotion_guidance": review_info.get("promotion_guidance", []),
            "next_step": next_step,
        }
        if next_step == "continue_to_pending_merge":
            benchmark_candidates.append(candidate)
        else:
            deferred_candidates.append(
                {
                    **candidate,
                    "candidate_type": "benchmark",
                    "deferred_reason": f"Deferred because next_step={next_step}.",
                }
            )

    case_candidates = _sort_candidates_by_quality_score(case_candidates)
    log_candidates = _sort_candidates_by_quality_score(log_candidates)
    benchmark_candidates = _sort_candidates_by_quality_score(benchmark_candidates)
    deferred_candidates = _sort_candidates_by_quality_score(deferred_candidates)

    plan_payload = {
        "pending_root": str(pending_root),
        "case_candidates": case_candidates,
        "log_candidates": log_candidates,
        "benchmark_candidates": benchmark_candidates,
        "deferred_candidates": deferred_candidates,
        "deferred_by_warning_category": _group_deferred_candidates_by_warning_category(deferred_candidates),
        "deferred_warning_category_summary": _summarize_deferred_warning_categories(deferred_candidates),
        "counts": {
            "eligible_case_candidates": len(case_candidates),
            "eligible_log_candidates": len(log_candidates),
            "eligible_benchmark_candidates": len(benchmark_candidates),
            "deferred_candidates": len(deferred_candidates),
        },
    }

    plan_json_path = plan_dir / "merge_plan.json"
    plan_json_path.write_text(json.dumps(plan_payload, ensure_ascii=False, indent=2), encoding="utf-8")
    plan_md_path = plan_dir / "merge_plan.md"
    plan_md_path.write_text(_render_pending_merge_plan_markdown(plan_payload), encoding="utf-8")

    return {
        "pending_root": str(pending_root),
        "output_dir": str(plan_dir),
        "merge_plan_json": str(plan_json_path),
        "merge_plan_markdown": str(plan_md_path),
    }


def prepare_formal_merge_assistant(root: Path) -> dict[str, object]:
    pending_root = root / "data" / "pending"
    assistant_dir = pending_root / "formal_merge_assistant"
    assistant_dir.mkdir(parents=True, exist_ok=True)

    merge_plan = plan_pending_merge(root)
    merge_plan_payload = json.loads(Path(str(merge_plan["merge_plan_json"])).read_text(encoding="utf-8"))

    case_files = _resolve_merge_plan_sources(merge_plan_payload.get("case_candidates", []))
    log_files = _resolve_merge_plan_sources(merge_plan_payload.get("log_candidates", []))
    benchmark_files = _resolve_merge_plan_sources(merge_plan_payload.get("benchmark_candidates", []))

    case_bundle_path = assistant_dir / "materials_case_merge_candidates.md"
    case_bundle_path.write_text(_render_case_merge_bundle(case_files), encoding="utf-8")

    log_bundle_path = assistant_dir / "log_merge_candidates.jsonl"
    _write_jsonl(
        log_bundle_path,
        [json.loads(path.read_text(encoding="utf-8")) for path in log_files],
    )

    benchmark_append_path = assistant_dir / "benchmark_append_candidates.jsonl"
    _write_jsonl(
        benchmark_append_path,
        [json.loads(path.read_text(encoding="utf-8")) for path in benchmark_files],
    )

    material_index_patch_path = assistant_dir / "material_index_patch.md"
    material_index_patch_path.write_text(
        _render_material_index_patch(case_files=case_files, log_files=log_files),
        encoding="utf-8",
    )

    assistant_payload = {
        "pending_root": str(pending_root),
        "generated_at": datetime.now().astimezone().isoformat(),
        "formal_targets": {
            "materials_dir": "data/materials/",
            "material_index": "data/materials/material_index_v1.md",
            "benchmark_jsonl": "data/benchmark/benchmark_v1.jsonl",
        },
        "merge_plan_json": str(merge_plan["merge_plan_json"]),
        "deferred_candidates": merge_plan_payload.get("deferred_candidates", []),
        "case_candidates": [
            {
                "source": str(path),
                "suggested_target": "data/materials/",
                "suggested_action": "review_then_copy_or_merge_section",
            }
            for path in case_files
        ],
        "log_candidates": [
            {
                "source": str(path),
                "suggested_target": "data/materials/ or future structured log corpus",
                "suggested_action": "review_then_reclassify",
            }
            for path in log_files
        ],
        "benchmark_candidates": [
            {
                "source": str(path),
                "suggested_target": "data/benchmark/benchmark_v1.jsonl",
                "suggested_action": "review_then_append_jsonl",
            }
            for path in benchmark_files
        ],
        "generated_files": {
            "materials_case_merge_candidates": str(case_bundle_path),
            "log_merge_candidates_jsonl": str(log_bundle_path),
            "benchmark_append_candidates_jsonl": str(benchmark_append_path),
            "material_index_patch": str(material_index_patch_path),
        },
        "counts": {
            "case_candidates": len(case_files),
            "log_candidates": len(log_files),
            "benchmark_candidates": len(benchmark_files),
            "deferred_candidates": len(merge_plan_payload.get("deferred_candidates", [])),
        },
    }

    assistant_json_path = assistant_dir / "formal_merge_assistant.json"
    assistant_json_path.write_text(json.dumps(assistant_payload, ensure_ascii=False, indent=2), encoding="utf-8")

    assistant_md_path = assistant_dir / "formal_merge_assistant.md"
    assistant_md_path.write_text(
        _render_formal_merge_assistant_markdown(assistant_payload, merge_plan_payload=merge_plan_payload),
        encoding="utf-8",
    )

    return {
        "pending_root": str(pending_root),
        "output_dir": str(assistant_dir),
        "formal_merge_assistant_json": str(assistant_json_path),
        "formal_merge_assistant_markdown": str(assistant_md_path),
        "materials_case_merge_candidates": str(case_bundle_path),
        "log_merge_candidates_jsonl": str(log_bundle_path),
        "benchmark_append_candidates_jsonl": str(benchmark_append_path),
        "material_index_patch": str(material_index_patch_path),
        "merge_plan_json": str(merge_plan["merge_plan_json"]),
    }


def apply_formal_merge(root: Path, *, dry_run: bool = True) -> dict[str, object]:
    pending_root = root / "data" / "pending"
    assistant = prepare_formal_merge_assistant(root)
    assistant_dir = Path(str(assistant["output_dir"]))
    assistant_payload = json.loads(Path(str(assistant["formal_merge_assistant_json"])).read_text(encoding="utf-8"))

    case_files = _resolve_merge_plan_sources(assistant_payload.get("case_candidates", []))
    log_files = _resolve_merge_plan_sources(assistant_payload.get("log_candidates", []))
    benchmark_files = _resolve_merge_plan_sources(assistant_payload.get("benchmark_candidates", []))
    deferred_candidates = assistant_payload.get("deferred_candidates", [])
    has_eligible_candidates = bool(case_files or log_files or benchmark_files)

    benchmark_target = root / "data" / "benchmark" / "benchmark_v1.jsonl"
    benchmark_existing_count = 0
    if benchmark_target.exists():
        benchmark_existing_count = len([line for line in benchmark_target.read_text(encoding="utf-8").splitlines() if line.strip()])

    benchmark_append_preview = []
    benchmark_payloads = []
    for path in benchmark_files:
        payload = json.loads(path.read_text(encoding="utf-8"))
        benchmark_payloads.append(payload)
        benchmark_append_preview.append(
            {
                "source": str(path),
                "candidate_id": payload.get("id", ""),
                "item_type": payload.get("item_type", ""),
                "target_file": "data/benchmark/benchmark_v1.jsonl",
                "suggested_append_after_line": benchmark_existing_count,
            }
        )

    material_copy_preview = [
        {
            "source": str(path),
            "target_dir": "data/materials/",
            "suggested_filename": path.name,
            "action": "copy_or_merge_curated_sections",
        }
        for path in case_files
    ]

    material_index_updates = []
    for path in case_files:
        material_index_updates.append(
            {
                "source": str(path),
                "proposed_entry": f"PENDING-{path.stem.upper()} | case_candidate | data/pending/cases/{path.name}",
            }
        )
    for path in log_files:
        payload = json.loads(path.read_text(encoding="utf-8"))
        material_index_updates.append(
            {
                "source": str(path),
                "proposed_entry": f"PENDING-{path.stem.upper()} | log_candidate | {payload.get('suggested_tag', '')} | data/pending/logs/{path.name}",
            }
        )

    result_payload = {
        "dry_run": dry_run,
        "status": "ready_for_apply" if has_eligible_candidates else "no_eligible_candidates",
        "should_continue": has_eligible_candidates,
        "pending_root": str(pending_root),
        "assistant_dir": str(assistant_dir),
        "formal_targets": {
            "materials_dir": "data/materials/",
            "material_index": "data/materials/material_index_v1.md",
            "benchmark_jsonl": "data/benchmark/benchmark_v1.jsonl",
        },
        "deferred_candidates": deferred_candidates,
        "planned_actions": {
            "case_material_promotions": material_copy_preview,
            "material_index_updates": material_index_updates,
            "benchmark_appends": benchmark_append_preview,
        },
        "notes": [
            "Dry-run only: no canonical dataset files were modified.",
            "Review candidate wording before copying case notes into data/materials/.",
            "Review benchmark candidate phrasing before appending into benchmark_v1.jsonl.",
        ],
        "recommended_commit_split": [
            {
                "name": "formal-material-candidates",
                "scope": [
                    "data/materials/",
                    "data/materials/material_index_v1.md",
                ],
                "reason": "Keep curated material imports reviewable and separate from benchmark changes.",
            },
            {
                "name": "formal-benchmark-appends",
                "scope": [
                    "data/benchmark/benchmark_v1.jsonl",
                ],
                "reason": "Keep benchmark candidate acceptance explicit and independently reviewable.",
            },
        ],
    }
    if not has_eligible_candidates:
        result_payload["notes"].insert(
            0,
            "No eligible candidates are ready for formal merge yet. Resolve deferred candidates before continuing.",
        )

    result_json_path = assistant_dir / "apply_formal_merge_dry_run.json"
    result_json_path.write_text(json.dumps(result_payload, ensure_ascii=False, indent=2), encoding="utf-8")

    result_md_path = assistant_dir / "apply_formal_merge_dry_run.md"
    result_md_path.write_text(_render_apply_formal_merge_markdown(result_payload), encoding="utf-8")

    benchmark_patch_path = assistant_dir / "benchmark_append_patch.jsonl"
    _write_jsonl(benchmark_patch_path, benchmark_payloads)

    material_index_patch_lines_path = assistant_dir / "material_index_append_patch.md"
    material_index_patch_lines_path.write_text(
        _render_material_index_append_patch_lines(material_index_updates),
        encoding="utf-8",
    )

    commit_plan_path = assistant_dir / "recommended_commit_split.md"
    commit_plan_path.write_text(
        _render_commit_split_markdown(result_payload["recommended_commit_split"]),
        encoding="utf-8",
    )

    staging_dir = assistant_dir / "staging"
    staged_files: dict[str, str] = {}
    if not dry_run:
        staging_materials_dir = staging_dir / "data" / "materials"
        staging_benchmark_dir = staging_dir / "data" / "benchmark"
        staging_materials_dir.mkdir(parents=True, exist_ok=True)
        staging_benchmark_dir.mkdir(parents=True, exist_ok=True)

        case_bundle_stage = staging_materials_dir / "materials_case_merge_candidates.md"
        shutil.copyfile(assistant_dir / "materials_case_merge_candidates.md", case_bundle_stage)
        staged_files["materials_case_merge_candidates"] = str(case_bundle_stage)

        material_index_stage = staging_materials_dir / "material_index_append_patch.md"
        shutil.copyfile(material_index_patch_lines_path, material_index_stage)
        staged_files["material_index_append_patch"] = str(material_index_stage)

        benchmark_stage = staging_benchmark_dir / "benchmark_append_patch.jsonl"
        shutil.copyfile(benchmark_patch_path, benchmark_stage)
        staged_files["benchmark_append_patch"] = str(benchmark_stage)

        commit_plan_stage = staging_dir / "recommended_commit_split.md"
        shutil.copyfile(commit_plan_path, commit_plan_stage)
        staged_files["recommended_commit_split"] = str(commit_plan_stage)

        staged_summary_path = staging_dir / "staging_summary.json"
        staged_summary_payload = {
            "mode": "staging_execute",
            "status": result_payload["status"],
            "should_continue": has_eligible_candidates,
            "staging_root": str(staging_dir),
            "staged_files": staged_files,
            "deferred_candidates": deferred_candidates,
            "notes": [
                "Canonical dataset files were not modified.",
                "Review staged patch files before any manual canonical merge.",
            ],
        }
        if not has_eligible_candidates:
            staged_summary_payload["notes"].insert(
                0,
                "No eligible candidates were staged for canonical merge. Resolve deferred candidates first.",
            )
        staged_summary_path.parent.mkdir(parents=True, exist_ok=True)
        staged_summary_path.write_text(json.dumps(staged_summary_payload, ensure_ascii=False, indent=2), encoding="utf-8")
        staged_files["staging_summary"] = str(staged_summary_path)

    return {
        "dry_run": dry_run,
        "output_dir": str(assistant_dir),
        "apply_formal_merge_json": str(result_json_path),
        "apply_formal_merge_markdown": str(result_md_path),
        "formal_merge_assistant_json": str(assistant["formal_merge_assistant_json"]),
        "benchmark_append_patch": str(benchmark_patch_path),
        "material_index_append_patch": str(material_index_patch_lines_path),
        "recommended_commit_split": str(commit_plan_path),
        "staging_root": str(staging_dir) if not dry_run else "",
        "staged_files": staged_files,
    }


def canonical_merge_preflight(root: Path) -> dict[str, object]:
    assistant_dir = root / "data" / "pending" / "formal_merge_assistant"
    assistant_dir.mkdir(parents=True, exist_ok=True)
    pending_root = root / "data" / "pending"
    case_files = sorted((pending_root / "cases").glob("*.md")) if (pending_root / "cases").exists() else []
    log_files = sorted((pending_root / "logs").glob("*.json")) if (pending_root / "logs").exists() else []
    benchmark_files = sorted(
        [path for path in (pending_root / "benchmarks").glob("*.json") if path.name != "pending_benchmark_candidates.jsonl"]
    ) if (pending_root / "benchmarks").exists() else []

    benchmark_target = root / "data" / "benchmark" / "benchmark_v1.jsonl"
    material_index_target = root / "data" / "materials" / "material_index_v1.md"
    staging_root = assistant_dir / "staging"
    benchmark_patch = assistant_dir / "benchmark_append_patch.jsonl"
    material_index_patch = assistant_dir / "material_index_append_patch.md"

    benchmark_existing_ids: set[str] = set()
    if benchmark_target.exists():
        for line in benchmark_target.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            try:
                payload = json.loads(line)
            except json.JSONDecodeError:
                continue
            candidate_id = str(payload.get("id", "")).strip()
            if candidate_id:
                benchmark_existing_ids.add(candidate_id)

    benchmark_patch_payloads: list[dict[str, object]] = []
    duplicate_benchmark_ids: list[str] = []
    if benchmark_patch.exists():
        for line in benchmark_patch.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            payload = json.loads(line)
            benchmark_patch_payloads.append(payload)
            candidate_id = str(payload.get("id", "")).strip()
            if candidate_id and candidate_id in benchmark_existing_ids:
                duplicate_benchmark_ids.append(candidate_id)

    material_index_text = material_index_target.read_text(encoding="utf-8") if material_index_target.exists() else ""
    proposed_material_entries: list[str] = []
    conflicting_material_entries: list[str] = []
    if material_index_patch.exists():
        for line in material_index_patch.read_text(encoding="utf-8").splitlines():
            stripped = line.strip()
            if not stripped.startswith("- "):
                continue
            entry = stripped[2:]
            proposed_material_entries.append(entry)
            if entry and entry in material_index_text:
                conflicting_material_entries.append(entry)

    expected_staging_files = [
        staging_root / "data" / "materials" / "materials_case_merge_candidates.md",
        staging_root / "data" / "materials" / "material_index_append_patch.md",
        staging_root / "data" / "benchmark" / "benchmark_append_patch.jsonl",
        staging_root / "recommended_commit_split.md",
        staging_root / "staging_summary.json",
    ]
    missing_staging_files = [str(path) for path in expected_staging_files if not path.exists()]

    checks = [
        {
            "name": "pending_candidates_present",
            "passed": (len(case_files) + len(log_files) + len(benchmark_files)) > 0,
            "details": {
                "case_candidates": len(case_files),
                "log_candidates": len(log_files),
                "benchmark_candidates": len(benchmark_files),
            },
        },
        {
            "name": "benchmark_target_exists",
            "passed": benchmark_target.exists(),
            "details": str(benchmark_target),
        },
        {
            "name": "material_index_target_exists",
            "passed": material_index_target.exists(),
            "details": str(material_index_target),
        },
        {
            "name": "benchmark_duplicate_ids",
            "passed": len(duplicate_benchmark_ids) == 0,
            "details": duplicate_benchmark_ids,
        },
        {
            "name": "material_index_conflicts",
            "passed": len(conflicting_material_entries) == 0,
            "details": conflicting_material_entries,
        },
        {
            "name": "staging_bundle_ready",
            "passed": len(missing_staging_files) == 0,
            "details": missing_staging_files,
        },
    ]

    payload = {
        "passed": all(item["passed"] for item in checks),
        "generated_at": datetime.now().astimezone().isoformat(),
        "assistant_dir": str(assistant_dir),
        "checks": checks,
        "benchmark_patch_count": len(benchmark_patch_payloads),
        "proposed_material_entry_count": len(proposed_material_entries),
    }

    json_path = assistant_dir / "canonical_merge_preflight.json"
    json_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    md_path = assistant_dir / "canonical_merge_preflight.md"
    md_path.write_text(_render_canonical_merge_preflight_markdown(payload), encoding="utf-8")

    return {
        "passed": payload["passed"],
        "output_dir": str(assistant_dir),
        "canonical_merge_preflight_json": str(json_path),
        "canonical_merge_preflight_markdown": str(md_path),
    }


def canonical_patch_helper(root: Path) -> dict[str, object]:
    assistant_dir = root / "data" / "pending" / "formal_merge_assistant"
    patch_dir = assistant_dir / "canonical_patch_bundle"
    patch_dir.mkdir(parents=True, exist_ok=True)

    benchmark_patch_src = assistant_dir / "benchmark_append_patch.jsonl"
    material_index_patch_src = assistant_dir / "material_index_append_patch.md"
    materials_case_src = assistant_dir / "materials_case_merge_candidates.md"
    commit_split_src = assistant_dir / "recommended_commit_split.md"

    benchmark_target = root / "data" / "benchmark" / "benchmark_v1.jsonl"
    material_index_target = root / "data" / "materials" / "material_index_v1.md"

    materials_patch_dir = patch_dir / "data" / "materials"
    benchmark_patch_dir = patch_dir / "data" / "benchmark"
    materials_patch_dir.mkdir(parents=True, exist_ok=True)
    benchmark_patch_dir.mkdir(parents=True, exist_ok=True)

    benchmark_patch_dst = benchmark_patch_dir / "benchmark_v1.append.jsonl"
    material_index_patch_dst = materials_patch_dir / "material_index_v1.append.md"
    materials_case_dst = materials_patch_dir / "materials_case_merge_candidates.md"
    commit_split_dst = patch_dir / "recommended_commit_split.md"

    if benchmark_patch_src.exists():
        shutil.copyfile(benchmark_patch_src, benchmark_patch_dst)
    else:
        benchmark_patch_dst.write_text("", encoding="utf-8")

    if material_index_patch_src.exists():
        shutil.copyfile(material_index_patch_src, material_index_patch_dst)
    else:
        material_index_patch_dst.write_text("# Material Index Append Patch\n\n- none\n", encoding="utf-8")

    if materials_case_src.exists():
        shutil.copyfile(materials_case_src, materials_case_dst)
    else:
        materials_case_dst.write_text("# Pending Case Merge Candidates\n\n- none\n", encoding="utf-8")

    if commit_split_src.exists():
        shutil.copyfile(commit_split_src, commit_split_dst)
    else:
        commit_split_dst.write_text("# Recommended Commit Split\n\n- none\n", encoding="utf-8")

    manifest = {
        "generated_at": datetime.now().astimezone().isoformat(),
        "assistant_dir": str(assistant_dir),
        "canonical_targets": {
            "benchmark": str(benchmark_target),
            "material_index": str(material_index_target),
            "materials_dir": str(root / "data" / "materials"),
        },
        "patch_files": {
            "benchmark_append_patch": str(benchmark_patch_dst),
            "material_index_append_patch": str(material_index_patch_dst),
            "materials_case_candidates": str(materials_case_dst),
            "recommended_commit_split": str(commit_split_dst),
        },
        "notes": [
            "This bundle does not modify canonical files.",
            "Review these patch files before any manual canonical merge.",
        ],
    }

    manifest_json = patch_dir / "canonical_patch_manifest.json"
    manifest_json.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")

    manifest_md = patch_dir / "canonical_patch_manifest.md"
    manifest_md.write_text(_render_canonical_patch_manifest_markdown(manifest), encoding="utf-8")

    return {
        "output_dir": str(patch_dir),
        "canonical_patch_manifest_json": str(manifest_json),
        "canonical_patch_manifest_markdown": str(manifest_md),
        "benchmark_append_patch": str(benchmark_patch_dst),
        "material_index_append_patch": str(material_index_patch_dst),
        "materials_case_candidates": str(materials_case_dst),
        "recommended_commit_split": str(commit_split_dst),
    }


def canonical_merge_preview_bundle(root: Path) -> dict[str, object]:
    assistant_dir = root / "data" / "pending" / "formal_merge_assistant"
    canonical_patch_helper(root)

    patch_dir = assistant_dir / "canonical_patch_bundle"
    preview_dir = assistant_dir / "canonical_merge_preview"
    preview_dir.mkdir(parents=True, exist_ok=True)

    benchmark_target = root / "data" / "benchmark" / "benchmark_v1.jsonl"
    material_index_target = root / "data" / "materials" / "material_index_v1.md"
    benchmark_patch = patch_dir / "data" / "benchmark" / "benchmark_v1.append.jsonl"
    material_index_patch = patch_dir / "data" / "materials" / "material_index_v1.append.md"
    materials_case_candidates = patch_dir / "data" / "materials" / "materials_case_merge_candidates.md"
    commit_split_src = patch_dir / "recommended_commit_split.md"

    preview_materials_dir = preview_dir / "data" / "materials"
    preview_benchmark_dir = preview_dir / "data" / "benchmark"
    preview_materials_dir.mkdir(parents=True, exist_ok=True)
    preview_benchmark_dir.mkdir(parents=True, exist_ok=True)

    benchmark_preview_path = preview_benchmark_dir / "benchmark_v1.preview.jsonl"
    benchmark_existing = benchmark_target.read_text(encoding="utf-8").rstrip()
    benchmark_append = benchmark_patch.read_text(encoding="utf-8").strip() if benchmark_patch.exists() else ""
    benchmark_preview_text = benchmark_existing
    if benchmark_append:
        benchmark_preview_text = f"{benchmark_existing}\n{benchmark_append}".strip() + "\n"
    elif benchmark_preview_text:
        benchmark_preview_text += "\n"
    benchmark_preview_path.write_text(benchmark_preview_text, encoding="utf-8")

    material_index_preview_path = preview_materials_dir / "material_index_v1.preview.md"
    material_index_existing = material_index_target.read_text(encoding="utf-8").rstrip()
    append_lines = []
    if material_index_patch.exists():
        for line in material_index_patch.read_text(encoding="utf-8").splitlines():
            stripped = line.strip()
            if stripped.startswith("- "):
                append_lines.append(stripped)
    material_index_preview_text = material_index_existing
    if append_lines:
        material_index_preview_text = material_index_preview_text + "\n\n## Pending Preview Entries\n\n" + "\n".join(append_lines) + "\n"
    elif material_index_preview_text:
        material_index_preview_text += "\n"
    material_index_preview_path.write_text(material_index_preview_text, encoding="utf-8")

    materials_case_preview_path = preview_materials_dir / "materials_case_merge_candidates.preview.md"
    if materials_case_candidates.exists():
        shutil.copyfile(materials_case_candidates, materials_case_preview_path)
    else:
        materials_case_preview_path.write_text("# Pending Case Merge Candidates\n\n- none\n", encoding="utf-8")

    commit_split_preview_path = preview_dir / "recommended_commit_split.preview.md"
    if commit_split_src.exists():
        shutil.copyfile(commit_split_src, commit_split_preview_path)
    else:
        commit_split_preview_path.write_text("# Recommended Commit Split\n\n- none\n", encoding="utf-8")

    preview_manifest = {
        "generated_at": datetime.now().astimezone().isoformat(),
        "assistant_dir": str(assistant_dir),
        "preview_root": str(preview_dir),
        "preview_files": {
            "benchmark_preview": str(benchmark_preview_path),
            "material_index_preview": str(material_index_preview_path),
            "materials_case_preview": str(materials_case_preview_path),
            "recommended_commit_split_preview": str(commit_split_preview_path),
        },
        "notes": [
            "These files preview canonical merge results without modifying canonical data.",
            "Preview files may still need human curation before any real canonical merge.",
        ],
    }

    preview_manifest_json = preview_dir / "canonical_merge_preview_manifest.json"
    preview_manifest_json.write_text(json.dumps(preview_manifest, ensure_ascii=False, indent=2), encoding="utf-8")

    preview_manifest_md = preview_dir / "canonical_merge_preview_manifest.md"
    preview_manifest_md.write_text(_render_canonical_merge_preview_manifest_markdown(preview_manifest), encoding="utf-8")

    return {
        "output_dir": str(preview_dir),
        "canonical_merge_preview_manifest_json": str(preview_manifest_json),
        "canonical_merge_preview_manifest_markdown": str(preview_manifest_md),
        "benchmark_preview": str(benchmark_preview_path),
        "material_index_preview": str(material_index_preview_path),
        "materials_case_preview": str(materials_case_preview_path),
        "recommended_commit_split_preview": str(commit_split_preview_path),
    }


def canonical_merge_report(root: Path) -> dict[str, object]:
    assistant_dir = root / "data" / "pending" / "formal_merge_assistant"
    report_dir = assistant_dir / "canonical_merge_report"
    report_dir.mkdir(parents=True, exist_ok=True)

    preflight = canonical_merge_preflight(root)
    patch_bundle = canonical_patch_helper(root)
    preview_bundle = canonical_merge_preview_bundle(root)

    report_payload = {
        "generated_at": datetime.now().astimezone().isoformat(),
        "assistant_dir": str(assistant_dir),
        "overall_ready_for_manual_canonical_review": bool(preflight.get("passed", False)),
        "review_order": [
            "1. Review canonical_merge_preflight.md for readiness and conflicts.",
            "2. Review canonical_patch_manifest.md and the generated append patch files.",
            "3. Review canonical_merge_preview_manifest.md and the preview files.",
            "4. Only after manual review, decide whether to apply curated changes into canonical files.",
        ],
        "preflight": {
            "passed": bool(preflight.get("passed", False)),
            "json": preflight.get("canonical_merge_preflight_json", ""),
            "markdown": preflight.get("canonical_merge_preflight_markdown", ""),
            "benchmark_patch_count": preflight.get("benchmark_patch_count", 0),
            "proposed_material_entry_count": preflight.get("proposed_material_entry_count", 0),
        },
        "patch_bundle": {
            "root": patch_bundle.get("output_dir", ""),
            "manifest_json": patch_bundle.get("canonical_patch_manifest_json", ""),
            "manifest_markdown": patch_bundle.get("canonical_patch_manifest_markdown", ""),
            "benchmark_append_patch": patch_bundle.get("benchmark_append_patch", ""),
            "material_index_append_patch": patch_bundle.get("material_index_append_patch", ""),
            "materials_case_candidates": patch_bundle.get("materials_case_candidates", ""),
            "recommended_commit_split": patch_bundle.get("recommended_commit_split", ""),
        },
        "preview_bundle": {
            "root": preview_bundle.get("output_dir", ""),
            "manifest_json": preview_bundle.get("canonical_merge_preview_manifest_json", ""),
            "manifest_markdown": preview_bundle.get("canonical_merge_preview_manifest_markdown", ""),
            "benchmark_preview": preview_bundle.get("benchmark_preview", ""),
            "material_index_preview": preview_bundle.get("material_index_preview", ""),
            "materials_case_preview": preview_bundle.get("materials_case_preview", ""),
            "recommended_commit_split_preview": preview_bundle.get("recommended_commit_split_preview", ""),
        },
        "notes": [
            "This report does not modify canonical files.",
            "Use it as a single overview page before any manual canonical merge decision.",
            "If preflight is not ready, review pending candidates and staging outputs before continuing.",
        ],
    }

    report_json = report_dir / "canonical_merge_report.json"
    report_json.write_text(json.dumps(report_payload, ensure_ascii=False, indent=2), encoding="utf-8")

    report_md = report_dir / "canonical_merge_report.md"
    report_md.write_text(_render_canonical_merge_report_markdown(report_payload), encoding="utf-8")

    return {
        "output_dir": str(report_dir),
        "canonical_merge_report_json": str(report_json),
        "canonical_merge_report_markdown": str(report_md),
        "preflight_markdown": str(preflight.get("canonical_merge_preflight_markdown", "")),
        "patch_manifest_markdown": str(patch_bundle.get("canonical_patch_manifest_markdown", "")),
        "preview_manifest_markdown": str(preview_bundle.get("canonical_merge_preview_manifest_markdown", "")),
    }


def canonical_merge_checklist(root: Path) -> dict[str, object]:
    assistant_dir = root / "data" / "pending" / "formal_merge_assistant"
    checklist_dir = assistant_dir / "canonical_merge_checklist"
    checklist_dir.mkdir(parents=True, exist_ok=True)

    preflight = canonical_merge_preflight(root)
    patch_bundle = canonical_patch_helper(root)
    preview_bundle = canonical_merge_preview_bundle(root)
    report = canonical_merge_report(root)

    checklist_items = [
        {
            "name": "preflight_passed",
            "checked": bool(preflight.get("passed", False)),
            "details": preflight.get("canonical_merge_preflight_markdown", ""),
        },
        {
            "name": "patch_bundle_ready",
            "checked": Path(str(patch_bundle.get("canonical_patch_manifest_markdown", ""))).exists(),
            "details": patch_bundle.get("canonical_patch_manifest_markdown", ""),
        },
        {
            "name": "preview_bundle_ready",
            "checked": Path(str(preview_bundle.get("canonical_merge_preview_manifest_markdown", ""))).exists(),
            "details": preview_bundle.get("canonical_merge_preview_manifest_markdown", ""),
        },
        {
            "name": "report_ready",
            "checked": Path(str(report.get("canonical_merge_report_markdown", ""))).exists(),
            "details": report.get("canonical_merge_report_markdown", ""),
        },
    ]

    blockers = []
    if not checklist_items[0]["checked"]:
        blockers.append("Preflight has not passed yet. Review pending candidates, duplicate IDs, and staging readiness first.")
    if not checklist_items[1]["checked"]:
        blockers.append("Canonical patch bundle is missing.")
    if not checklist_items[2]["checked"]:
        blockers.append("Canonical merge preview bundle is missing.")
    if not checklist_items[3]["checked"]:
        blockers.append("Canonical merge report is missing.")

    recommendation = (
        "Manual canonical merge can be reviewed now."
        if not blockers
        else "Do not perform manual canonical merge yet. Resolve the blockers first."
    )
    next_steps = (
        [
            "Open canonical_merge_report.md first.",
            "Then inspect canonical_patch_manifest.md and canonical_merge_preview_manifest.md.",
            "If all content still looks correct after human review, prepare a curated manual merge commit.",
        ]
        if not blockers
        else [
            "Open canonical_merge_preflight.md and resolve any failed checks.",
            "Regenerate patch, preview, and report outputs after fixing blockers.",
            "Re-run canonical-merge-checklist before attempting any manual merge.",
        ]
    )

    payload = {
        "generated_at": datetime.now().astimezone().isoformat(),
        "assistant_dir": str(assistant_dir),
        "ready_for_manual_canonical_merge_review": not blockers,
        "checklist_items": checklist_items,
        "blockers": blockers,
        "recommendation": recommendation,
        "next_steps": next_steps,
        "key_outputs": {
            "preflight_markdown": preflight.get("canonical_merge_preflight_markdown", ""),
            "patch_manifest_markdown": patch_bundle.get("canonical_patch_manifest_markdown", ""),
            "preview_manifest_markdown": preview_bundle.get("canonical_merge_preview_manifest_markdown", ""),
            "report_markdown": report.get("canonical_merge_report_markdown", ""),
        },
    }

    checklist_json = checklist_dir / "canonical_merge_checklist.json"
    checklist_json.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    checklist_md = checklist_dir / "canonical_merge_checklist.md"
    checklist_md.write_text(_render_canonical_merge_checklist_markdown(payload), encoding="utf-8")

    return {
        "output_dir": str(checklist_dir),
        "canonical_merge_checklist_json": str(checklist_json),
        "canonical_merge_checklist_markdown": str(checklist_md),
        "ready_for_manual_canonical_merge_review": payload["ready_for_manual_canonical_merge_review"],
    }


def build_bench_pack(
    root: Path,
    request: str,
    *,
    tool_id: str | None = None,
    session_id: str | None = None,
    label: str | None = None,
    execute: bool = True,
    timeout_seconds: int = 20,
    output_path: Path | None = None,
) -> dict[str, object]:
    timestamp = datetime.now().astimezone()
    resolved_session_id = _resolve_session_id(session_id, label, timestamp)
    mode = get_execution_mode(root)
    doctor = inspect_wsl_environment(root)
    result = run_tool_request(
        root,
        request,
        tool_id=tool_id,
        execute=execute,
        timeout_seconds=timeout_seconds,
    )

    pack = {
        "pack_type": "bench_pack_v1",
        "captured_at": timestamp.isoformat(),
        "request": request,
        "tool_id_override": tool_id,
        "session_id": resolved_session_id,
        "label": label,
        "mode": mode,
        "doctor": doctor,
        "result": result,
    }

    destination = output_path or _default_bench_pack_path(root, timestamp, session_id=resolved_session_id)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(pack, ensure_ascii=False, indent=2), encoding="utf-8")
    pack["saved_to"] = str(destination)
    return pack


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


def _read_stub_scenario(root: Path) -> str:
    profile_path = root / WSL_STUB_PROFILE
    if not profile_path.exists():
        return DEFAULT_STUB_SCENARIO
    try:
        payload = json.loads(profile_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return DEFAULT_STUB_SCENARIO
    return _normalize_stub_scenario(str(payload.get("scenario", DEFAULT_STUB_SCENARIO)))


def _normalize_stub_scenario(value: str) -> str:
    return value if value in STUB_SCENARIOS else DEFAULT_STUB_SCENARIO


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


def _default_bench_pack_path(root: Path, timestamp: datetime, *, session_id: str | None = None) -> Path:
    stamp = timestamp.strftime("%Y%m%d_%H%M%S_%f")
    suffix = uuid4().hex[:8]
    if session_id:
        return root / "reports" / "bench_packs" / "sessions" / session_id / f"bench_pack_{stamp}_{suffix}.json"
    return root / "reports" / "bench_packs" / f"bench_pack_{stamp}_{suffix}.json"


def _resolve_session_id(session_id: str | None, label: str | None, timestamp: datetime) -> str | None:
    if session_id:
        normalized = _slugify_token(session_id)
        return normalized or None
    if label:
        normalized = _slugify_token(label)
        if normalized:
            return f"{normalized}_{timestamp.strftime('%Y%m%d')}"
    return None


def _render_real_bench_index(
    session_id: str,
    label: str | None,
    timestamp: datetime,
    *,
    doctor: dict[str, object],
    git_context: dict[str, str],
) -> str:
    lines = [
        "# Real Bench Prep Pack",
        "",
        f"- Session ID: {session_id}",
        f"- Session label: {label or ''}",
        f"- Generated at: {timestamp.isoformat()}",
        f"- Git branch: {git_context.get('branch', '')}",
        f"- Git commit: {git_context.get('commit', '')}",
        "",
        "## Current Runtime Snapshot",
        "",
        f"- execution_mode: {doctor.get('execution_mode', '')}",
        f"- stub_mode_enabled: {doctor.get('stub_mode_enabled', '')}",
        f"- real_mode_ready: {doctor.get('real_mode_ready', '')}",
        f"- stub_scenario: {doctor.get('stub_scenario', '')}",
        f"- wsl_available: {doctor.get('wsl_available', '')}",
        f"- python3_available: {doctor.get('python3_available', '')}",
        f"- stub_library_present: {doctor.get('stub_library_present', '')}",
        "- Machine-readable snapshot: `doctor_snapshot.json`",
        "- Machine-readable first-step seed: `plan_seed.json`",
        "",
        "## Included Files",
        "",
        "- `00_index.md`: Startup guide and recommended order.",
        "- `01_readiness_checklist.md`: Pre-bench readiness checklist.",
        "- `02_first_run_record.md`: First read-only run template.",
        "- `03_issue_capture.md`: Immediate issue capture template.",
        "- `04_session_review.md`: Session review template.",
        "- `doctor_snapshot.json`: Runtime and git snapshot captured when this prep pack was generated.",
        "- `plan_seed.json`: Canonical readonly startup request for kickoff automation.",
        "",
        "## Recommended Order",
        "",
        "1. Read `01_readiness_checklist.md` before touching the bench.",
        "2. Fill `02_first_run_record.md` during the first read-only run.",
        "3. If something goes wrong, switch to `03_issue_capture.md` immediately.",
        "4. End the session with `04_session_review.md`.",
        "",
        "## Suggested First Commands",
        "",
        "```powershell",
        "python -m industrial_embedded_dev_agent tools use-real",
        "python -m industrial_embedded_dev_agent tools doctor",
        "python -m industrial_embedded_dev_agent tools plan \"先读一下 axis0/axis1 的状态字、错误码和编码器，我要看当前链路是不是还活着。\"",
        "python -m industrial_embedded_dev_agent tools run \"先读一下 axis0/axis1 的状态字、错误码和编码器，我要看当前链路是不是还活着。\" --execute",
        "```",
        "",
        "The same first-step commands are also stored in `plan_seed.json` for machine-readable reuse.",
        "",
        "## Preferred Kickoff Path",
        "",
        "Once the runtime checks look correct, prefer using the generated seed file instead of typing the readonly request manually:",
        "",
        "```powershell",
        f"python -m industrial_embedded_dev_agent tools kickoff-real-bench \"reports/real_bench_prep/{session_id}/plan_seed.json\" --render-all",
        "```",
        "",
        "This one command will generate the first `bench-pack` plus both markdown drafts:",
        "- `first-run`",
        "- `session-review`",
        "",
        "Use `doctor_snapshot.json` to compare the original prep-time environment against the live bench environment before executing.",
        "",
    ]
    return "\n".join(lines)


def _archive_kickoff_outputs(seed_path: Path, payload: dict[str, object]) -> dict[str, object] | None:
    prep_dir = seed_path.parent
    if not prep_dir.exists():
        return None

    archive_dir = prep_dir / "kickoff_outputs"
    archive_dir.mkdir(parents=True, exist_ok=True)

    bench_pack = payload.get("bench_pack", {})
    bench_pack_path = Path(str(bench_pack.get("saved_to", ""))) if bench_pack.get("saved_to") else None
    first_run = payload.get("rendered_first_run", {})
    session_review = payload.get("rendered_session_review", {})
    first_run_path = Path(str(first_run.get("output_path", ""))) if first_run.get("output_path") else None
    session_review_path = Path(str(session_review.get("output_path", ""))) if session_review.get("output_path") else None

    archived_files: list[str] = []
    copied_bench_pack_path = _copy_if_present(bench_pack_path, archive_dir / "bench_pack.json")
    if copied_bench_pack_path:
        archived_files.append(str(copied_bench_pack_path))
    copied_first_run_path = _copy_if_present(first_run_path, archive_dir / "first_run.md")
    if copied_first_run_path:
        archived_files.append(str(copied_first_run_path))
    copied_session_review_path = _copy_if_present(session_review_path, archive_dir / "session_review.md")
    if copied_session_review_path:
        archived_files.append(str(copied_session_review_path))

    summary_payload = {
        "session_id": payload.get("session_id", ""),
        "label": payload.get("label", ""),
        "seed_path": str(seed_path),
        "execute": payload.get("execute", False),
        "request": payload.get("request", ""),
        "bench_pack_path": str(copied_bench_pack_path) if copied_bench_pack_path else "",
        "first_run_path": str(copied_first_run_path) if copied_first_run_path else "",
        "session_review_path": str(copied_session_review_path) if copied_session_review_path else "",
        "plan": bench_pack.get("result", {}).get("plan", {}),
        "parsed_output": bench_pack.get("result", {}).get("execution", {}).get("parsed_output", {}),
    }
    summary_json_path = archive_dir / "run_summary.json"
    summary_json_path.write_text(json.dumps(summary_payload, ensure_ascii=False, indent=2), encoding="utf-8")
    archived_files.append(str(summary_json_path))

    summary_md_path = archive_dir / "run_summary.md"
    summary_md_path.write_text(_render_kickoff_summary_markdown(summary_payload), encoding="utf-8")
    archived_files.append(str(summary_md_path))

    return {
        "output_dir": str(archive_dir),
        "files": archived_files,
        "run_summary_json": str(summary_json_path),
        "run_summary_markdown": str(summary_md_path),
    }


def _copy_if_present(source: Path | None, destination: Path) -> Path | None:
    if source is None or not source.exists():
        return None
    shutil.copyfile(source, destination)
    return destination


def _render_kickoff_summary_markdown(summary: dict[str, object]) -> str:
    plan = summary.get("plan", {})
    parsed = summary.get("parsed_output", {})
    lines = [
        "# Real Bench Kickoff Summary",
        "",
        f"- Session ID: {summary.get('session_id', '')}",
        f"- Session label: {summary.get('label', '')}",
        f"- Seed path: {summary.get('seed_path', '')}",
        f"- Execute: {summary.get('execute', False)}",
        "",
        "## Request",
        "",
        f"- {summary.get('request', '')}",
        "",
        "## Plan Snapshot",
        "",
        f"- tool_id: {plan.get('tool_id', '')}",
        f"- risk_level: {plan.get('risk_level', '')}",
        f"- allowed_to_execute: {plan.get('allowed_to_execute', '')}",
        "",
        "## Parsed Output Snapshot",
        "",
        f"- status: {parsed.get('status', '')}",
        f"- summary: {parsed.get('summary', '')}",
        f"- transport_state: {parsed.get('transport_state', '')}",
        f"- next_action: {parsed.get('next_action', '')}",
        "",
        "## Archived Outputs",
        "",
        f"- bench_pack.json: {summary.get('bench_pack_path', '')}",
        f"- first_run.md: {summary.get('first_run_path', '')}",
        f"- session_review.md: {summary.get('session_review_path', '')}",
        "",
    ]
    return "\n".join(lines)


def _render_finish_summary_markdown(summary: dict[str, object]) -> str:
    kickoff_summary = summary.get("kickoff_summary", {})
    plan = kickoff_summary.get("plan", {})
    parsed = kickoff_summary.get("parsed_output", {})
    lines = [
        "# Real Bench Finish Summary",
        "",
        f"- Session ID: {summary.get('session_id', '')}",
        f"- Prep dir: {summary.get('prep_dir', '')}",
        f"- Pack count: {summary.get('pack_count', 0)}",
        f"- kickoff_outputs_present: {summary.get('kickoff_outputs_present', False)}",
        "",
        "## Aggregated Outputs",
        "",
        f"- session_bundle.md: {summary.get('session_bundle_path', '')}",
        f"- latest_compare.md: {summary.get('latest_compare_path', '')}",
        f"- final_summary.json: {summary.get('final_summary_json', '')}",
        "",
        "## Latest Kickoff Snapshot",
        "",
        f"- tool_id: {plan.get('tool_id', '')}",
        f"- risk_level: {plan.get('risk_level', '')}",
        f"- parsed status: {parsed.get('status', '')}",
        f"- transport_state: {parsed.get('transport_state', '')}",
        f"- next_action: {parsed.get('next_action', '')}",
        "",
    ]
    return "\n".join(lines)


def _export_finish_candidates(
    finish_dir: Path,
    *,
    session_id: str,
    summary_payload: dict[str, object],
) -> dict[str, object]:
    candidate_dir = finish_dir / "candidate_exports"
    candidate_dir.mkdir(parents=True, exist_ok=True)

    kickoff_summary = summary_payload.get("kickoff_summary", {})
    plan = kickoff_summary.get("plan", {})
    parsed = kickoff_summary.get("parsed_output", {})
    issue_tag = _candidate_issue_tag(parsed, plan)

    case_path = candidate_dir / "case_candidate.md"
    case_path.write_text(
        "\n".join(
            [
                "# Case Candidate",
                "",
                f"- Session ID: {session_id}",
                f"- Suggested tag: {issue_tag}",
                f"- Tool ID: {plan.get('tool_id', '')}",
                f"- Risk level: {plan.get('risk_level', '')}",
                "",
                "## Observed Summary",
                "",
                f"- {parsed.get('summary', '')}",
                "",
                "## Why This Matters",
                "",
                "- This candidate was generated from a real-bench finish pack and should be reviewed before entering the formal case library.",
                "",
                "## Suggested Next Step",
                "",
                f"- {parsed.get('next_action', '')}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    log_candidate = {
        "session_id": session_id,
        "suggested_tag": issue_tag,
        "tool_id": plan.get("tool_id", ""),
        "risk_level": plan.get("risk_level", ""),
        "parsed_output": parsed,
    }
    log_path = candidate_dir / "log_candidate.json"
    log_path.write_text(json.dumps(log_candidate, ensure_ascii=False, indent=2), encoding="utf-8")

    benchmark_candidate = {
        "id": f"candidate-{session_id}",
        "item_type": "log_attribution",
        "input": {
            "question": f"{session_id} finish pack 里暴露的主要问题更像什么？",
        },
        "expected": {
            "must_include": [issue_tag],
        },
        "tags": [issue_tag, "finish_pack_candidate"],
        "difficulty": "medium",
    }
    benchmark_path = candidate_dir / "benchmark_candidate.json"
    benchmark_path.write_text(json.dumps(benchmark_candidate, ensure_ascii=False, indent=2), encoding="utf-8")

    return {
        "output_dir": str(candidate_dir),
        "files": [str(case_path), str(log_path), str(benchmark_path)],
        "case_candidate": str(case_path),
        "log_candidate": str(log_path),
        "benchmark_candidate": str(benchmark_path),
    }


def _candidate_issue_tag(parsed: dict[str, object], plan: dict[str, object]) -> str:
    transport_state = str(parsed.get("transport_state", "")).strip()
    status = str(parsed.get("status", "")).strip()
    summary = str(parsed.get("summary", "")).lower()
    if transport_state == "unverified":
        return "transport_degraded"
    if status in {"environment_error", "execution_failed"}:
        return "environment_blocker"
    if "error code" in summary or "non-zero" in summary:
        return "axis_fault_snapshot"
    if str(plan.get("tool_id", "")) == "SCRIPT-004":
        return "readonly_snapshot_review"
    return "finish_pack_review"


def _evaluate_case_candidate(path: Path, text: str) -> dict[str, object]:
    warnings: list[str] = []
    warning_details: list[dict[str, str]] = []
    exists = path.exists()
    if not exists:
        _append_warning(warnings, warning_details, "missing_file", "case candidate file is missing")
        return {
            "exists": False,
            "passed": False,
            "warnings": warnings,
            "warning_details": warning_details,
            "warning_categories": _warning_categories_from_details(warning_details),
            "length": 0,
            "quality_level": "blocked",
            "quality_score": 0,
        }

    normalized = text.strip()
    length = len(normalized)
    if length < 120:
        _append_warning(warnings, warning_details, "weak_content", "case candidate content is very short")
    if "## Observed Summary" not in text:
        _append_warning(warnings, warning_details, "missing_fields", "case candidate is missing the observed summary section")
    if "## Suggested Next Step" not in text:
        _append_warning(warnings, warning_details, "missing_fields", "case candidate is missing the suggested next step section")

    return {
        "exists": True,
        "passed": len(warnings) == 0,
        "warnings": warnings,
        "warning_details": warning_details,
        "warning_categories": _warning_categories_from_details(warning_details),
        "length": length,
        "quality_level": _quality_level(exists=True, warnings=warnings),
        "quality_score": _quality_score(exists=True, warnings=warnings),
    }


def _evaluate_log_candidate(path: Path, payload: dict[str, object]) -> dict[str, object]:
    warnings: list[str] = []
    warning_details: list[dict[str, str]] = []
    exists = path.exists()
    if not exists:
        _append_warning(warnings, warning_details, "missing_file", "log candidate file is missing")
        return {
            "exists": False,
            "passed": False,
            "warnings": warnings,
            "warning_details": warning_details,
            "warning_categories": _warning_categories_from_details(warning_details),
            "quality_level": "blocked",
            "quality_score": 0,
        }

    required_fields = ["session_id", "suggested_tag", "tool_id", "risk_level", "parsed_output"]
    missing_fields = [field for field in required_fields if not payload.get(field)]
    if missing_fields:
        _append_warning(
            warnings,
            warning_details,
            "missing_fields",
            f"log candidate is missing required fields: {', '.join(missing_fields)}",
        )

    suggested_tag = str(payload.get("suggested_tag", "")).strip()
    if not suggested_tag:
        _append_warning(warnings, warning_details, "missing_fields", "log candidate suggested_tag is empty")
    elif len(suggested_tag) < 4:
        _append_warning(warnings, warning_details, "review_noise", "log candidate suggested_tag looks too weak")

    parsed_output = payload.get("parsed_output", {})
    if not isinstance(parsed_output, dict) or not parsed_output:
        _append_warning(warnings, warning_details, "missing_fields", "log candidate parsed_output is empty")
    elif not str(parsed_output.get("summary", "")).strip():
        _append_warning(warnings, warning_details, "weak_content", "log candidate parsed_output.summary is empty")

    return {
        "exists": True,
        "passed": len(warnings) == 0,
        "warnings": warnings,
        "warning_details": warning_details,
        "warning_categories": _warning_categories_from_details(warning_details),
        "required_fields_present": len(missing_fields) == 0,
        "quality_level": _quality_level(
            exists=True,
            warnings=warnings,
            critical_failed=bool(missing_fields),
        ),
        "quality_score": _quality_score(
            exists=True,
            warnings=warnings,
            critical_failed=bool(missing_fields),
        ),
    }


def _evaluate_benchmark_candidate(path: Path, payload: dict[str, object]) -> dict[str, object]:
    warnings: list[str] = []
    warning_details: list[dict[str, str]] = []
    exists = path.exists()
    if not exists:
        _append_warning(warnings, warning_details, "missing_file", "benchmark candidate file is missing")
        return {
            "exists": False,
            "passed": False,
            "warnings": warnings,
            "warning_details": warning_details,
            "warning_categories": _warning_categories_from_details(warning_details),
            "quality_level": "blocked",
            "quality_score": 0,
        }

    candidate_id = str(payload.get("id", "")).strip()
    item_type = str(payload.get("item_type", "")).strip()
    question = str(payload.get("input", {}).get("question", "")).strip()
    tags = payload.get("tags", [])
    must_include = payload.get("expected", {}).get("must_include", [])

    if not candidate_id:
        _append_warning(warnings, warning_details, "missing_fields", "benchmark candidate id is empty")
    elif not re.fullmatch(r"candidate-[a-z0-9][a-z0-9_-]*", candidate_id):
        _append_warning(
            warnings,
            warning_details,
            "review_noise",
            "benchmark candidate id does not match the expected candidate-* style",
        )

    if not item_type:
        _append_warning(warnings, warning_details, "missing_fields", "benchmark candidate item_type is empty")
    if not question:
        _append_warning(warnings, warning_details, "missing_fields", "benchmark candidate question is empty")
    elif len(question) < 12:
        _append_warning(warnings, warning_details, "weak_content", "benchmark candidate question is too short")
    if not isinstance(tags, list) or not tags:
        _append_warning(warnings, warning_details, "missing_fields", "benchmark candidate tags are empty")
    if not isinstance(must_include, list) or not must_include:
        _append_warning(warnings, warning_details, "missing_fields", "benchmark candidate expected.must_include is empty")

    return {
        "exists": True,
        "passed": len(warnings) == 0,
        "warnings": warnings,
        "warning_details": warning_details,
        "warning_categories": _warning_categories_from_details(warning_details),
        "candidate_id": candidate_id,
        "quality_level": _quality_level(
            exists=True,
            warnings=warnings,
            critical_failed=not candidate_id or not item_type or not question,
        ),
        "quality_score": _quality_score(
            exists=True,
            warnings=warnings,
            critical_failed=not candidate_id or not item_type or not question,
        ),
    }


def _review_recommendation(review_payload: dict[str, object]) -> str:
    has_all_candidates = bool(
        review_payload.get("has_case_candidate")
        and review_payload.get("has_log_candidate")
        and review_payload.get("has_benchmark_candidate")
    )
    quality_present = bool(review_payload.get("quality_check_present"))
    quality_passed = bool(review_payload.get("quality_overall_passed"))
    quality_level = str(review_payload.get("quality_level", "")).strip()
    quality_warnings = review_payload.get("quality_warnings", [])

    if not has_all_candidates:
        return "hold_for_manual_analysis"
    if quality_level == "blocked":
        return "hold_for_manual_analysis"
    if quality_present and quality_passed:
        return "promote_now"
    if quality_level == "weak" or (quality_present and quality_warnings):
        return "edit_before_promote"
    return "hold_for_manual_analysis"


def _append_warning(
    warnings: list[str],
    warning_details: list[dict[str, str]],
    category: str,
    message: str,
) -> None:
    warnings.append(message)
    warning_details.append({"category": category, "message": message})


def _warning_categories_from_details(warning_details: list[dict[str, str]]) -> list[str]:
    categories: list[str] = []
    for detail in warning_details:
        category = str(detail.get("category", "")).strip()
        if category and category not in categories:
            categories.append(category)
    return categories


def _collect_warning_categories(results: list[dict[str, object]]) -> list[str]:
    categories: list[str] = []
    for result in results:
        for category in result.get("warning_categories", []):
            normalized = str(category).strip()
            if normalized and normalized not in categories:
                categories.append(normalized)
    return categories


def _guidance_for_warning_categories(categories: object) -> list[str]:
    if not isinstance(categories, list):
        return []

    guidance_map = {
        "missing_file": "补齐缺失 candidate 文件后，再进入 review 和 promotion。",
        "missing_fields": "先补齐关键字段和必要 section，再继续推进。",
        "weak_content": "先加强摘要、问题描述或证据表达，再继续推进。",
        "review_noise": "先清理命名、标签或低价值噪音信息，再继续推进。",
    }

    guidance: list[str] = []
    for category in categories:
        normalized = str(category).strip()
        message = guidance_map.get(normalized)
        if message and message not in guidance:
            guidance.append(message)
    return guidance


def _load_promotion_records(promotion_records_dir: Path) -> dict[str, dict[str, object]]:
    if not promotion_records_dir.exists():
        return {}

    records: dict[str, dict[str, object]] = {}
    for path in sorted(promotion_records_dir.glob("*_promotion_record.json")):
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        session_id = str(payload.get("session_id", "")).strip()
        if session_id:
            records[session_id] = payload
    return records


def _quality_level(*, exists: bool, warnings: list[str], critical_failed: bool = False) -> str:
    if not exists or critical_failed:
        return "blocked"
    if warnings:
        return "weak"
    return "good"


def _quality_score(*, exists: bool, warnings: list[str], critical_failed: bool = False) -> int:
    if not exists:
        return 0
    score = 100
    if critical_failed:
        score -= 60
    score -= 10 * len(warnings)
    if score < 0:
        return 0
    if score > 100:
        return 100
    return score


def _overall_quality_level(results: list[dict[str, object]]) -> str:
    levels = [str(result.get("quality_level", "")).strip() for result in results]
    if any(level == "blocked" for level in levels):
        return "blocked"
    if any(level == "weak" for level in levels):
        return "weak"
    return "good"


def _overall_quality_score(results: list[dict[str, object]]) -> int:
    scores = [int(result.get("quality_score", 0)) for result in results]
    if not scores:
        return 0
    return int(sum(scores) / len(scores))


def _sort_candidates_by_quality_score(candidates: list[dict[str, object]]) -> list[dict[str, object]]:
    return sorted(
        candidates,
        key=lambda item: (
            -int(item.get("quality_score", 0)),
            str(item.get("session_id", "")),
            str(item.get("source", "")),
        ),
    )


def _build_top_review_candidates(
    case_candidates: list[dict[str, object]],
    log_candidates: list[dict[str, object]],
    benchmark_candidates: list[dict[str, object]],
    deferred_candidates: list[dict[str, object]],
    *,
    limit: int = 5,
) -> list[dict[str, object]]:
    items: list[dict[str, object]] = []
    for candidate in case_candidates:
        items.append({**candidate, "review_bucket": "eligible"})
    for candidate in log_candidates:
        items.append({**candidate, "review_bucket": "eligible"})
    for candidate in benchmark_candidates:
        items.append({**candidate, "review_bucket": "eligible"})
    for candidate in deferred_candidates:
        items.append({**candidate, "review_bucket": "deferred"})
    sorted_items = _sort_candidates_by_quality_score(items)[:limit]
    return [
        {
            **item,
            "review_priority": _review_priority_label(item),
        }
        for item in sorted_items
    ]


def _group_deferred_candidates_by_warning_category(
    deferred_candidates: list[dict[str, object]],
) -> list[dict[str, object]]:
    grouped: dict[str, list[dict[str, object]]] = {}
    for item in deferred_candidates:
        categories = item.get("quality_warning_categories", [])
        if isinstance(categories, list) and categories:
            category = str(categories[0]).strip() or "uncategorized"
        else:
            category = "uncategorized"
        grouped.setdefault(category, []).append(item)

    ordered_groups: list[dict[str, object]] = []
    for category in sorted(grouped):
        ordered_groups.append(
            {
                "category": category,
                "items": _sort_candidates_by_quality_score(grouped[category]),
            }
        )
    return ordered_groups


def _summarize_deferred_warning_categories(
    deferred_candidates: list[dict[str, object]],
) -> list[dict[str, object]]:
    groups = _group_deferred_candidates_by_warning_category(deferred_candidates)
    summary: list[dict[str, object]] = []
    for group in groups:
        items = group.get("items", [])
        scores = [int(item.get("quality_score", 0)) for item in items]
        summary.append(
            {
                "category": group.get("category", "uncategorized"),
                "count": len(items),
                "max_score": max(scores) if scores else 0,
                "min_score": min(scores) if scores else 0,
            }
        )
    return summary


def _review_priority_label(item: dict[str, object]) -> str:
    review_bucket = str(item.get("review_bucket", "")).strip()
    quality_level = str(item.get("quality_level", "")).strip()
    next_step = str(item.get("next_step", "")).strip()

    if review_bucket == "eligible":
        return "review_now"
    if quality_level == "blocked" or next_step == "stop_and_analyze":
        return "blocked"
    return "watch"


def _resolve_merge_plan_sources(entries: object) -> list[Path]:
    if not isinstance(entries, list):
        return []

    resolved: list[Path] = []
    for item in entries:
        if not isinstance(item, dict):
            continue
        source = str(item.get("source", "")).strip()
        if not source:
            continue
        path = Path(source)
        if path.exists():
            resolved.append(path)
    return resolved


def _render_finish_candidate_review_markdown(
    review_payload: dict[str, object],
    *,
    case_preview: str,
    benchmark_payload: dict[str, object],
    quality_payload: dict[str, object],
) -> str:
    preview_lines = case_preview.splitlines()[:12]
    benchmark_tags = benchmark_payload.get("tags", [])
    benchmark_question = benchmark_payload.get("input", {}).get("question", "")
    quality_warnings = quality_payload.get("warnings", [])
    lines = [
        "# Finish Candidate Review",
        "",
        f"- Session ID: {review_payload.get('session_id', '')}",
        f"- candidate_dir: {review_payload.get('candidate_dir', '')}",
        f"- suggested_tag: {review_payload.get('suggested_tag', '')}",
        f"- tool_id: {review_payload.get('tool_id', '')}",
        f"- risk_level: {review_payload.get('risk_level', '')}",
        f"- review_recommendation: {review_payload.get('review_recommendation', '')}",
        "",
        "## Candidate Presence",
        "",
        f"- case_candidate: {review_payload.get('has_case_candidate', False)}",
        f"- log_candidate: {review_payload.get('has_log_candidate', False)}",
        f"- benchmark_candidate: {review_payload.get('has_benchmark_candidate', False)}",
        "",
        "## Candidate Quality Check",
        "",
        f"- quality_check_present: {review_payload.get('quality_check_present', False)}",
        f"- quality_overall_passed: {review_payload.get('quality_overall_passed', False)}",
        f"- quality_level: {review_payload.get('quality_level', '')}",
        f"- quality_score: {review_payload.get('quality_score', 0)}",
        f"- quality_warning_categories: {', '.join(review_payload.get('quality_warning_categories', []))}",
        f"- quality_summary_path: {review_payload.get('quality_summary_path', '')}",
        "",
        "### Quality Warnings",
        "",
        "## Benchmark Preview",
        "",
        f"- item_type: {review_payload.get('benchmark_item_type', '')}",
        f"- question: {benchmark_question}",
        f"- tags: {', '.join(benchmark_tags) if benchmark_tags else ''}",
        "",
        "## Case Preview",
        "",
        *preview_lines,
        "",
        "## Reviewer Checklist",
        "",
        "- Does the suggested tag match the actual bench evidence?",
        "- Is the benchmark question phrased clearly enough for future regression use?",
        "- Should this candidate stay as a draft, be edited, or be promoted into the formal dataset?",
        f"- Current machine recommendation: {review_payload.get('review_recommendation', '')}",
        "",
    ]
    if quality_warnings:
        insertion_index = lines.index("## Benchmark Preview")
        quality_lines = [f"- {warning}" for warning in quality_warnings]
        lines[insertion_index:insertion_index] = quality_lines + [""]
    else:
        insertion_index = lines.index("## Benchmark Preview")
        lines[insertion_index:insertion_index] = ["- none", ""]

    quality_guidance = review_payload.get("quality_guidance", [])
    guidance_block = ["### Quality Guidance", ""]
    if quality_guidance:
        guidance_block.extend([f"- {item}" for item in quality_guidance])
    else:
        guidance_block.append("- none")
    guidance_block.append("")
    insertion_index = lines.index("## Benchmark Preview")
    lines[insertion_index:insertion_index] = guidance_block
    return "\n".join(lines)


def _render_promotion_record_markdown(promotion_record: dict[str, object]) -> str:
    lines = [
        "# Promotion Record",
        "",
        f"- session_id: {promotion_record.get('session_id', '')}",
        f"- prep_dir: {promotion_record.get('prep_dir', '')}",
        f"- candidate_dir: {promotion_record.get('candidate_dir', '')}",
        f"- review_summary_path: {promotion_record.get('review_summary_path', '')}",
        f"- review_recommendation: {promotion_record.get('review_recommendation', '')}",
        f"- quality_warning_categories: {', '.join(promotion_record.get('quality_warning_categories', []))}",
        f"- soft_blocked: {promotion_record.get('soft_blocked', False)}",
        f"- next_step: {promotion_record.get('next_step', '')}",
        f"- promotion_warning: {promotion_record.get('promotion_warning', '')}",
        "",
        "## Promotion Guidance",
        "",
    ]
    promotion_guidance = promotion_record.get("promotion_guidance", [])
    if promotion_guidance:
        lines.extend([f"- {item}" for item in promotion_guidance])
    else:
        lines.append("- none")

    lines.extend([
        "",
        "## Promoted Files",
        "",
        f"- case: {promotion_record.get('promoted_case_path', '')}",
        f"- log: {promotion_record.get('promoted_log_path', '')}",
        f"- benchmark: {promotion_record.get('promoted_benchmark_path', '')}",
        f"- pending_benchmark_jsonl: {promotion_record.get('pending_benchmark_jsonl', '')}",
        "",
    ])
    return "\n".join(lines)


def _render_candidate_quality_check_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# Candidate Quality Check",
        "",
        f"- Session ID: {payload.get('session_id', '')}",
        f"- candidate_dir: {payload.get('candidate_dir', '')}",
        f"- overall_passed: {payload.get('overall_passed', False)}",
        f"- quality_level: {payload.get('quality_level', '')}",
        f"- quality_score: {payload.get('quality_score', 0)}",
        f"- warning_categories: {', '.join(payload.get('warning_categories', []))}",
        f"- next_action: {payload.get('next_action', '')}",
        "",
        "## Case Candidate",
        "",
        f"- exists: {payload.get('case_candidate', {}).get('exists', False)}",
        f"- passed: {payload.get('case_candidate', {}).get('passed', False)}",
        f"- quality_level: {payload.get('case_candidate', {}).get('quality_level', '')}",
        f"- quality_score: {payload.get('case_candidate', {}).get('quality_score', 0)}",
        f"- length: {payload.get('case_candidate', {}).get('length', 0)}",
        "",
        "## Log Candidate",
        "",
        f"- exists: {payload.get('log_candidate', {}).get('exists', False)}",
        f"- passed: {payload.get('log_candidate', {}).get('passed', False)}",
        f"- quality_level: {payload.get('log_candidate', {}).get('quality_level', '')}",
        f"- quality_score: {payload.get('log_candidate', {}).get('quality_score', 0)}",
        f"- required_fields_present: {payload.get('log_candidate', {}).get('required_fields_present', False)}",
        "",
        "## Benchmark Candidate",
        "",
        f"- exists: {payload.get('benchmark_candidate', {}).get('exists', False)}",
        f"- passed: {payload.get('benchmark_candidate', {}).get('passed', False)}",
        f"- quality_level: {payload.get('benchmark_candidate', {}).get('quality_level', '')}",
        f"- quality_score: {payload.get('benchmark_candidate', {}).get('quality_score', 0)}",
        f"- candidate_id: {payload.get('benchmark_candidate', {}).get('candidate_id', '')}",
        "",
        "## Warnings",
        "",
    ]
    warnings = payload.get("warnings", [])
    if warnings:
        for warning in warnings:
            lines.append(f"- {warning}")
    else:
        lines.append("- none")
    lines.extend(["", "## Warning Details", ""])
    warning_details = payload.get("warning_details", [])
    if warning_details:
        for detail in warning_details:
            lines.append(f"- [{detail.get('category', '')}] {detail.get('message', '')}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_pending_merge_plan_markdown(plan_payload: dict[str, object]) -> str:
    top_review_items = _build_top_review_candidates(
        plan_payload.get("case_candidates", []),
        plan_payload.get("log_candidates", []),
        plan_payload.get("benchmark_candidates", []),
        plan_payload.get("deferred_candidates", []),
    )
    lines = [
        "# Pending Merge Plan",
        "",
        f"- pending_root: {plan_payload.get('pending_root', '')}",
        f"- eligible_case_candidates: {plan_payload.get('counts', {}).get('eligible_case_candidates', 0)}",
        f"- eligible_log_candidates: {plan_payload.get('counts', {}).get('eligible_log_candidates', 0)}",
        f"- eligible_benchmark_candidates: {plan_payload.get('counts', {}).get('eligible_benchmark_candidates', 0)}",
        f"- deferred_candidates: {plan_payload.get('counts', {}).get('deferred_candidates', 0)}",
        "",
        "## Top Candidates To Review First",
        "",
    ]
    if top_review_items:
        for item in top_review_items:
            lines.append(
                f"- [{item.get('review_priority', '')}] [{item.get('review_bucket', '')}] {item.get('source', '')} "
                f"(quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')}; "
                f"next_step={item.get('next_step', '')})"
            )
    else:
        lines.append("- none")

    lines.extend([
        "",
        "## Case Candidates",
        "",
    ])
    case_candidates = plan_payload.get("case_candidates", [])
    if case_candidates:
        for item in case_candidates:
            lines.append(
                f"- {item.get('source', '')} -> {item.get('suggested_target', '')} "
                f"({item.get('action', '')}; quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')})"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Log Candidates", ""])
    log_candidates = plan_payload.get("log_candidates", [])
    if log_candidates:
        for item in log_candidates:
            lines.append(
                f"- {item.get('source', '')} -> {item.get('suggested_target', '')} "
                f"({item.get('action', '')}; quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')})"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Benchmark Candidates", ""])
    benchmark_candidates = plan_payload.get("benchmark_candidates", [])
    if benchmark_candidates:
        for item in benchmark_candidates:
            lines.append(
                f"- {item.get('source', '')} -> {item.get('suggested_target', '')} "
                f"({item.get('action', '')}; quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')})"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Deferred Candidates", ""])
    deferred_candidates = plan_payload.get("deferred_candidates", [])
    if deferred_candidates:
        for item in deferred_candidates:
            lines.append(
                f"- [{item.get('candidate_type', '')}] {item.get('source', '')} "
                f"({item.get('deferred_reason', '')}; quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')}; next_step={item.get('next_step', '')})"
            )
            guidance = item.get("promotion_guidance", [])
            if guidance:
                for message in guidance:
                    lines.append(f"  guidance: {message}")
    else:
        lines.append("- none")

    lines.extend(["", "## Deferred Candidates By Warning Category", ""])
    deferred_summary = plan_payload.get("deferred_warning_category_summary", [])
    if deferred_summary:
        lines.extend(["### Category Summary", ""])
        for item in deferred_summary:
            lines.append(
                f"- {item.get('category', 'uncategorized')}: "
                f"count={item.get('count', 0)}, "
                f"max_score={item.get('max_score', 0)}, "
                f"min_score={item.get('min_score', 0)}"
            )
        lines.append("")
    deferred_groups = plan_payload.get("deferred_by_warning_category", [])
    if deferred_groups:
        for group in deferred_groups:
            lines.append(f"### {group.get('category', 'uncategorized')}")
            for item in group.get("items", []):
                lines.append(
                    f"- [{item.get('candidate_type', '')}] {item.get('source', '')} "
                    f"(quality_level={item.get('quality_level', '')}; "
                    f"quality_score={item.get('quality_score', 0)}; "
                    f"next_step={item.get('next_step', '')})"
                )
            lines.append("")
    else:
        lines.append("- none")
        lines.append("")

    lines.extend(
        [
            "",
            "## Merge Guidance",
            "",
            "- Review pending case candidates before copying them into formal materials.",
            "- Review pending benchmark candidates before appending them into the canonical benchmark file.",
            "- Deferred candidates should not move forward until their next_step is cleared.",
            "- Keep promotion and formal merge as two separate commits whenever possible.",
            "",
        ]
    )
    return "\n".join(lines)


def _render_case_merge_bundle(case_files: list[Path]) -> str:
    lines = [
        "# Pending Case Merge Candidates",
        "",
        "This file is a review bundle for case candidates currently staged under `data/pending/cases/`.",
        "",
    ]
    if not case_files:
        lines.append("- none")
        lines.append("")
        return "\n".join(lines)

    for path in case_files:
        lines.extend(
            [
                f"## Source: {path.name}",
                "",
                f"- Suggested formal target: data/materials/",
                f"- Suggested action: review then merge or copy as a curated case note",
                "",
                path.read_text(encoding="utf-8").strip(),
                "",
            ]
        )
    return "\n".join(lines)


def _render_material_index_patch(*, case_files: list[Path], log_files: list[Path]) -> str:
    lines = [
        "# Suggested Material Index Patch",
        "",
        "Review these draft entries before appending them into `data/materials/material_index_v1.md`.",
        "",
        "## Suggested Case Entries",
        "",
    ]
    if case_files:
        for path in case_files:
            lines.append(
                f"- source_id: PENDING-{path.stem.upper()} | category: case_candidate | source: data/pending/cases/{path.name}"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Suggested Log Entries", ""])
    if log_files:
        for path in log_files:
            payload = json.loads(path.read_text(encoding="utf-8"))
            lines.append(
                f"- source_id: PENDING-{path.stem.upper()} | category: log_candidate | tag: {payload.get('suggested_tag', '')} | source: data/pending/logs/{path.name}"
            )
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_formal_merge_assistant_markdown(
    assistant_payload: dict[str, object],
    *,
    merge_plan_payload: dict[str, object],
) -> str:
    counts = assistant_payload.get("counts", {})
    generated_files = assistant_payload.get("generated_files", {})
    formal_targets = assistant_payload.get("formal_targets", {})
    top_review_items = _build_top_review_candidates(
        merge_plan_payload.get("case_candidates", []),
        merge_plan_payload.get("log_candidates", []),
        merge_plan_payload.get("benchmark_candidates", []),
        merge_plan_payload.get("deferred_candidates", []),
    )
    lines = [
        "# Formal Merge Assistant",
        "",
        f"- pending_root: {assistant_payload.get('pending_root', '')}",
        f"- generated_at: {assistant_payload.get('generated_at', '')}",
        "",
        "## Formal Targets",
        "",
        f"- materials_dir: {formal_targets.get('materials_dir', '')}",
        f"- material_index: {formal_targets.get('material_index', '')}",
        f"- benchmark_jsonl: {formal_targets.get('benchmark_jsonl', '')}",
        "",
        "## Candidate Counts",
        "",
        f"- case_candidates: {counts.get('case_candidates', 0)}",
        f"- log_candidates: {counts.get('log_candidates', 0)}",
        f"- benchmark_candidates: {counts.get('benchmark_candidates', 0)}",
        "",
        "## Generated Draft Files",
        "",
        f"- materials_case_merge_candidates: {generated_files.get('materials_case_merge_candidates', '')}",
        f"- log_merge_candidates_jsonl: {generated_files.get('log_merge_candidates_jsonl', '')}",
        f"- benchmark_append_candidates_jsonl: {generated_files.get('benchmark_append_candidates_jsonl', '')}",
        f"- material_index_patch: {generated_files.get('material_index_patch', '')}",
        "",
        "## Recommended Merge Order",
        "",
        "1. Review the pending case bundle and keep only curated, reusable case notes.",
        "2. Review the pending log candidates and decide whether they belong in materials or a future dedicated log corpus.",
        "3. Review the benchmark append candidates before touching the canonical benchmark file.",
        "4. Update the formal material index in a separate commit from the pending promotion step.",
        "",
        "## Top Candidates To Review First",
        "",
    ]
    if top_review_items:
        for item in top_review_items:
            lines.append(
                f"- [{item.get('review_priority', '')}] [{item.get('review_bucket', '')}] {item.get('source', '')} "
                f"(quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')}; "
                f"next_step={item.get('next_step', '')})"
            )
    else:
        lines.append("- none")

    lines.extend([
        "",
        "## Merge Plan Snapshot",
        "",
    ])
    for section_name in ("case_candidates", "log_candidates", "benchmark_candidates"):
        items = merge_plan_payload.get(section_name, [])
        lines.append(f"### {section_name}")
        if items:
            for item in items:
                lines.append(
                    f"- {item.get('source', '')} -> {item.get('suggested_target', '')} "
                    f"({item.get('action', '')}; quality_level={item.get('quality_level', '')}; "
                    f"quality_score={item.get('quality_score', 0)}; "
                    f"review_recommendation={item.get('review_recommendation', '')})"
                )
        else:
            lines.append("- none")
        lines.append("")

    deferred_candidates = merge_plan_payload.get("deferred_candidates", [])
    lines.extend(["### deferred_candidates"])
    if deferred_candidates:
        for item in deferred_candidates:
            lines.append(
                f"- [{item.get('candidate_type', '')}] {item.get('source', '')} "
                f"(quality_level={item.get('quality_level', '')}; "
                f"quality_score={item.get('quality_score', 0)}; "
                f"review_recommendation={item.get('review_recommendation', '')}; "
                f"next_step={item.get('next_step', '')})"
            )
            guidance = item.get("promotion_guidance", [])
            if guidance:
                for message in guidance:
                    lines.append(f"  guidance: {message}")
    else:
        lines.append("- none")
    lines.append("")

    lines.extend(["### deferred_by_warning_category"])
    deferred_summary = merge_plan_payload.get("deferred_warning_category_summary", [])
    if deferred_summary:
        lines.append("  summary:")
        for item in deferred_summary:
            lines.append(
                f"  - {item.get('category', 'uncategorized')}: "
                f"count={item.get('count', 0)}, "
                f"max_score={item.get('max_score', 0)}, "
                f"min_score={item.get('min_score', 0)}"
            )
    deferred_groups = merge_plan_payload.get("deferred_by_warning_category", [])
    if deferred_groups:
        for group in deferred_groups:
            lines.append(f"- category: {group.get('category', 'uncategorized')}")
            for item in group.get("items", []):
                lines.append(
                    f"  - [{item.get('candidate_type', '')}] {item.get('source', '')} "
                    f"(quality_level={item.get('quality_level', '')}; "
                    f"quality_score={item.get('quality_score', 0)}; "
                    f"next_step={item.get('next_step', '')})"
                )
        lines.append("")
    else:
        lines.append("- none")
        lines.append("")

    lines.extend(
        [
            "## Safety Boundary",
            "",
            "- This assistant does not edit formal dataset files directly.",
            "- It only prepares draft bundles and append-ready files for human-reviewed follow-up changes.",
            "",
        ]
    )
    return "\n".join(lines)


def _render_apply_formal_merge_markdown(payload: dict[str, object]) -> str:
    planned_actions = payload.get("planned_actions", {})
    case_items = planned_actions.get("case_material_promotions", [])
    index_items = planned_actions.get("material_index_updates", [])
    benchmark_items = planned_actions.get("benchmark_appends", [])
    deferred_items = payload.get("deferred_candidates", [])
    notes = payload.get("notes", [])

    lines = [
        "# Apply Formal Merge Dry Run",
        "",
        f"- dry_run: {payload.get('dry_run', True)}",
        f"- status: {payload.get('status', '')}",
        f"- should_continue: {payload.get('should_continue', False)}",
        f"- pending_root: {payload.get('pending_root', '')}",
        f"- assistant_dir: {payload.get('assistant_dir', '')}",
        "",
        "## Case Material Promotions",
        "",
    ]
    if case_items:
        for item in case_items:
            lines.append(
                f"- {item.get('source', '')} -> {item.get('target_dir', '')}{item.get('suggested_filename', '')} ({item.get('action', '')})"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Material Index Updates", ""])
    if index_items:
        for item in index_items:
            lines.append(f"- {item.get('proposed_entry', '')}")
    else:
        lines.append("- none")

    lines.extend(["", "## Benchmark Appends", ""])
    if benchmark_items:
        for item in benchmark_items:
            lines.append(
                f"- {item.get('candidate_id', '')} ({item.get('item_type', '')}) -> {item.get('target_file', '')} after line {item.get('suggested_append_after_line', '')}"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Deferred Candidates", ""])
    if deferred_items:
        for item in deferred_items:
            lines.append(
                f"- [{item.get('candidate_type', '')}] {item.get('source', '')} "
                f"(next_step={item.get('next_step', '')})"
            )
    else:
        lines.append("- none")

    lines.extend(["", "## Notes", ""])
    if notes:
        for note in notes:
            lines.append(f"- {note}")
    else:
        lines.append("- none")

    commit_split = payload.get("recommended_commit_split", [])
    lines.extend(["", "## Recommended Commit Split", ""])
    if commit_split:
        for item in commit_split:
            scope = ", ".join(item.get("scope", []))
            lines.append(f"- {item.get('name', '')}: {scope} | {item.get('reason', '')}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_canonical_merge_preflight_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# Canonical Merge Preflight",
        "",
        f"- passed: {payload.get('passed', False)}",
        f"- generated_at: {payload.get('generated_at', '')}",
        f"- assistant_dir: {payload.get('assistant_dir', '')}",
        "",
        f"- benchmark_patch_count: {payload.get('benchmark_patch_count', 0)}",
        f"- proposed_material_entry_count: {payload.get('proposed_material_entry_count', 0)}",
        "",
        "## Checks",
        "",
    ]
    for item in payload.get("checks", []):
        lines.append(f"- {item.get('name', '')}: passed={item.get('passed', False)} details={item.get('details', '')}")
    lines.append("")
    return "\n".join(lines)


def _render_canonical_patch_manifest_markdown(manifest: dict[str, object]) -> str:
    targets = manifest.get("canonical_targets", {})
    patch_files = manifest.get("patch_files", {})
    notes = manifest.get("notes", [])
    lines = [
        "# Canonical Patch Manifest",
        "",
        f"- generated_at: {manifest.get('generated_at', '')}",
        f"- assistant_dir: {manifest.get('assistant_dir', '')}",
        "",
        "## Canonical Targets",
        "",
        f"- benchmark: {targets.get('benchmark', '')}",
        f"- material_index: {targets.get('material_index', '')}",
        f"- materials_dir: {targets.get('materials_dir', '')}",
        "",
        "## Patch Files",
        "",
        f"- benchmark_append_patch: {patch_files.get('benchmark_append_patch', '')}",
        f"- material_index_append_patch: {patch_files.get('material_index_append_patch', '')}",
        f"- materials_case_candidates: {patch_files.get('materials_case_candidates', '')}",
        f"- recommended_commit_split: {patch_files.get('recommended_commit_split', '')}",
        "",
        "## Notes",
        "",
    ]
    if notes:
        for note in notes:
            lines.append(f"- {note}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_canonical_merge_preview_manifest_markdown(manifest: dict[str, object]) -> str:
    preview_files = manifest.get("preview_files", {})
    notes = manifest.get("notes", [])
    lines = [
        "# Canonical Merge Preview Manifest",
        "",
        f"- generated_at: {manifest.get('generated_at', '')}",
        f"- assistant_dir: {manifest.get('assistant_dir', '')}",
        f"- preview_root: {manifest.get('preview_root', '')}",
        "",
        "## Preview Files",
        "",
        f"- benchmark_preview: {preview_files.get('benchmark_preview', '')}",
        f"- material_index_preview: {preview_files.get('material_index_preview', '')}",
        f"- materials_case_preview: {preview_files.get('materials_case_preview', '')}",
        f"- recommended_commit_split_preview: {preview_files.get('recommended_commit_split_preview', '')}",
        "",
        "## Notes",
        "",
    ]
    if notes:
        for note in notes:
            lines.append(f"- {note}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_canonical_merge_report_markdown(payload: dict[str, object]) -> str:
    preflight = payload.get("preflight", {})
    patch_bundle = payload.get("patch_bundle", {})
    preview_bundle = payload.get("preview_bundle", {})
    review_order = payload.get("review_order", [])
    notes = payload.get("notes", [])
    lines = [
        "# Canonical Merge Report",
        "",
        f"- generated_at: {payload.get('generated_at', '')}",
        f"- assistant_dir: {payload.get('assistant_dir', '')}",
        f"- overall_ready_for_manual_canonical_review: {payload.get('overall_ready_for_manual_canonical_review', False)}",
        "",
        "## Preflight",
        "",
        f"- passed: {preflight.get('passed', False)}",
        f"- benchmark_patch_count: {preflight.get('benchmark_patch_count', 0)}",
        f"- proposed_material_entry_count: {preflight.get('proposed_material_entry_count', 0)}",
        f"- json: {preflight.get('json', '')}",
        f"- markdown: {preflight.get('markdown', '')}",
        "",
        "## Patch Bundle",
        "",
        f"- root: {patch_bundle.get('root', '')}",
        f"- manifest_json: {patch_bundle.get('manifest_json', '')}",
        f"- manifest_markdown: {patch_bundle.get('manifest_markdown', '')}",
        f"- benchmark_append_patch: {patch_bundle.get('benchmark_append_patch', '')}",
        f"- material_index_append_patch: {patch_bundle.get('material_index_append_patch', '')}",
        f"- materials_case_candidates: {patch_bundle.get('materials_case_candidates', '')}",
        f"- recommended_commit_split: {patch_bundle.get('recommended_commit_split', '')}",
        "",
        "## Preview Bundle",
        "",
        f"- root: {preview_bundle.get('root', '')}",
        f"- manifest_json: {preview_bundle.get('manifest_json', '')}",
        f"- manifest_markdown: {preview_bundle.get('manifest_markdown', '')}",
        f"- benchmark_preview: {preview_bundle.get('benchmark_preview', '')}",
        f"- material_index_preview: {preview_bundle.get('material_index_preview', '')}",
        f"- materials_case_preview: {preview_bundle.get('materials_case_preview', '')}",
        f"- recommended_commit_split_preview: {preview_bundle.get('recommended_commit_split_preview', '')}",
        "",
        "## Recommended Review Order",
        "",
    ]
    if review_order:
        for item in review_order:
            lines.append(f"- {item}")
    else:
        lines.append("- none")
    lines.extend(["", "## Notes", ""])
    if notes:
        for note in notes:
            lines.append(f"- {note}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_canonical_merge_checklist_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# Canonical Merge Checklist",
        "",
        f"- generated_at: {payload.get('generated_at', '')}",
        f"- assistant_dir: {payload.get('assistant_dir', '')}",
        f"- ready_for_manual_canonical_merge_review: {payload.get('ready_for_manual_canonical_merge_review', False)}",
        f"- recommendation: {payload.get('recommendation', '')}",
        "",
        "## Checklist Items",
        "",
    ]
    for item in payload.get("checklist_items", []):
        lines.append(
            f"- {item.get('name', '')}: checked={item.get('checked', False)} details={item.get('details', '')}"
        )
    lines.extend(["", "## Blockers", ""])
    blockers = payload.get("blockers", [])
    if blockers:
        for blocker in blockers:
            lines.append(f"- {blocker}")
    else:
        lines.append("- none")
    lines.extend(["", "## Next Steps", ""])
    for step in payload.get("next_steps", []):
        lines.append(f"- {step}")
    lines.append("")
    return "\n".join(lines)


def _render_material_index_append_patch_lines(items: list[dict[str, object]]) -> str:
    lines = [
        "# Material Index Append Patch",
        "",
        "Append the following reviewed lines into `data/materials/material_index_v1.md` only after curation.",
        "",
    ]
    if items:
        for item in items:
            lines.append(f"- {item.get('proposed_entry', '')}")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def _render_commit_split_markdown(items: list[dict[str, object]]) -> str:
    lines = [
        "# Recommended Commit Split",
        "",
    ]
    if items:
        for item in items:
            lines.extend(
                [
                    f"## {item.get('name', '')}",
                    "",
                    f"- scope: {', '.join(item.get('scope', []))}",
                    f"- reason: {item.get('reason', '')}",
                    "",
                ]
            )
    else:
        lines.append("- none")
        lines.append("")
    return "\n".join(lines)


def _write_jsonl(path: Path, payloads: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        for item in payloads:
            handle.write(json.dumps(item, ensure_ascii=False) + "\n")


def _collect_git_context(root: Path) -> dict[str, str]:
    return {
        "branch": _capture_git_value(root, ["rev-parse", "--abbrev-ref", "HEAD"]),
        "commit": _capture_git_value(root, ["rev-parse", "--short", "HEAD"]),
    }


def _capture_git_value(root: Path, args: list[str]) -> str:
    command = ["git", *args]
    completed = subprocess.run(command, cwd=str(root), capture_output=True, text=True)
    if completed.returncode != 0:
        return ""
    return completed.stdout.strip()


def _wrap_real_bench_doc(
    *,
    title: str,
    session_id: str,
    label: str | None,
    timestamp: datetime,
    source_path: Path,
) -> str:
    body = source_path.read_text(encoding="utf-8").strip()
    lines = [
        f"# {title}",
        "",
        f"- Session ID: {session_id}",
        f"- Session label: {label or ''}",
        f"- Generated at: {timestamp.isoformat()}",
        f"- Source template: {source_path.name}",
        "",
        "## Session Notes",
        "",
        "- Bench location:",
        "- Board / platform:",
        "- Servo drive:",
        "- Motor:",
        "- Current branch / commit:",
        "",
        "---",
        "",
        body,
        "",
    ]
    return "\n".join(lines)


def _slugify_token(value: str) -> str:
    lowered = value.strip().lower()
    slug = re.sub(r"[^0-9a-zA-Z\u4e00-\u9fff]+", "-", lowered)
    slug = slug.strip("-")
    return slug[:80]


def _default_rendered_pack_path(root: Path, input_path: Path, *, template: str) -> Path:
    base_name = input_path.stem
    safe_template = template.replace("_", "-")
    return root / "reports" / "bench_packs" / "rendered" / f"{base_name}_{safe_template}.md"


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


def _render_pack_template(pack: dict[str, object], *, template: str) -> str:
    if template == "first-run":
        return _render_first_run_template(pack)
    if template == "issue":
        return _render_issue_template(pack)
    raise ValueError(f"Unsupported template: {template}")


def _render_first_run_template(pack: dict[str, object]) -> str:
    normalized_pack = _normalize_render_payload(pack)
    captured_at = str(normalized_pack.get("captured_at", ""))
    request = str(normalized_pack.get("request", ""))
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
