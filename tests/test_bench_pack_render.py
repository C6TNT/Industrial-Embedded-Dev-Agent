from __future__ import annotations

import shutil
import json
from pathlib import Path

from industrial_embedded_dev_agent.bench_pack_render import (
    compare_bench_packs,
    compare_latest_bench_packs_in_session,
    render_bench_pack_markdown,
    render_session_bundle_markdown,
    summarize_bench_pack_diff_from_files,
    summarize_bench_sessions,
)
from industrial_embedded_dev_agent.models import BenchmarkItem
from industrial_embedded_dev_agent.runner import run_local_checks_with_options
from industrial_embedded_dev_agent.tools import apply_formal_merge, canonical_merge_checklist, canonical_merge_preflight, canonical_merge_preview_bundle, canonical_merge_report, canonical_patch_helper, finish_real_bench, kickoff_real_bench, plan_pending_merge, prepare_formal_merge_assistant, prepare_real_bench_package, promote_finish_candidates, review_finish_candidates


REPO_ROOT = Path(__file__).resolve().parents[1]
SAMPLES_ROOT = REPO_ROOT / "data" / "examples" / "stub_bench_packs"
PENDING_README_TEMPLATE = """# Pending Dataset Area

`data/pending/` is the review-and-promotion buffer between bench-generated candidate drafts and the formal project dataset.

It exists for one reason:
- bench automation can generate useful candidate artifacts quickly
- but those artifacts should not be merged into the formal dataset without human review

## What Goes Here

- `cases/`
  Candidate case summaries promoted from `finish-real-bench`
- `logs/`
  Candidate log-style structured evidence exported from bench sessions
- `benchmarks/`
  Candidate benchmark items and the aggregated `pending_benchmark_candidates.jsonl`
- `promotion_records/`
  Machine-readable records showing what was promoted, from which session, and where it landed

## What Does Not Belong Here

- raw runtime dumps that should stay under `reports/`
- temporary scratch files with no review value
- direct edits to the formal benchmark or material index without review

## Recommended Flow

1. Run `ieda tools finish-real-bench --session-id <id>`
2. Run `ieda tools review-finish-candidates --session-id <id>`
3. Manually inspect the generated review summary and candidate files
4. Run `ieda tools promote-finish-candidates --session-id <id>`
5. Periodically merge reviewed pending items into the formal dataset in a separate, explicit change

## Merge Rule

Promotion into `data/pending/` means:
- the candidate is considered useful enough to keep
- but it is still not a formal benchmark, formal case, or formal material entry

Formal dataset updates should happen in a separate reviewable step, so the repo always keeps a clear boundary between:
- generated candidate content
- curated canonical content
"""


def _copy_sample_to_session(tmp_path: Path, sample_name: str, session_id: str, target_name: str) -> Path:
    session_dir = tmp_path / "reports" / "bench_packs" / "sessions" / session_id
    session_dir.mkdir(parents=True, exist_ok=True)
    destination = session_dir / target_name
    shutil.copyfile(SAMPLES_ROOT / sample_name, destination)
    return destination


def _reset_pending_root() -> None:
    pending_root = REPO_ROOT / "data" / "pending"
    if pending_root.exists():
        shutil.rmtree(pending_root, ignore_errors=True)
    pending_root.mkdir(parents=True, exist_ok=True)
    (pending_root / "cases").mkdir(parents=True, exist_ok=True)
    (pending_root / "logs").mkdir(parents=True, exist_ok=True)
    (pending_root / "benchmarks").mkdir(parents=True, exist_ok=True)
    (pending_root / "promotion_records").mkdir(parents=True, exist_ok=True)
    (pending_root / "README.md").write_text(PENDING_README_TEMPLATE, encoding="utf-8")


def test_compare_pack_detects_axis1_fault_fields(tmp_path: Path) -> None:
    output_path = tmp_path / "axis_fault_compare.md"
    summary = compare_bench_packs(
        tmp_path,
        SAMPLES_ROOT / "sample_nominal.json",
        SAMPLES_ROOT / "sample_axis1_fault.json",
        output_path=output_path,
    )

    changed_axes = {item["axis_id"]: item for item in summary["axis_diffs"] if item["changed"]}
    assert output_path.exists()
    assert summary["same_session"] is True
    assert summary["transport_changed"] is False
    assert "1" in changed_axes
    assert "0" not in changed_axes
    assert changed_axes["1"]["fields"]["error_code"] == {"left": 0, "right": 16}
    assert changed_axes["1"]["fields"]["status_code"] == {"left": 33, "right": 144}
    assert changed_axes["1"]["fields"]["axis_status"] == {"left": 4097, "right": 9473}


def test_compare_pack_detects_open_rpmsg_transport_drop(tmp_path: Path) -> None:
    summary = compare_bench_packs(
        tmp_path,
        SAMPLES_ROOT / "sample_nominal.json",
        SAMPLES_ROOT / "sample_open_rpmsg_fail.json",
    )

    assert summary["transport_changed"] is True
    assert summary["left_transport_state"] == "readable"
    assert summary["right_transport_state"] == "unverified"
    assert any("Transport state changed" in item for item in summary["observations"])


def test_summarize_bench_pack_diff_from_files_matches_compare_summary() -> None:
    summary = summarize_bench_pack_diff_from_files(
        SAMPLES_ROOT / "sample_nominal.json",
        SAMPLES_ROOT / "sample_axis1_fault.json",
    )

    changed_axes = {item["axis_id"]: item for item in summary["axis_diffs"] if item["changed"]}
    assert summary["same_session"] is True
    assert summary["transport_changed"] is False
    assert list(changed_axes) == ["1"]
    assert changed_axes["1"]["fields"]["encoder"] == {"left": 2008, "right": 8}


def test_compare_latest_bench_packs_in_session_uses_last_two_sorted_files(tmp_path: Path) -> None:
    session_id = "stub-session"
    _copy_sample_to_session(tmp_path, "sample_nominal.json", session_id, "bench_pack_001.json")
    _copy_sample_to_session(tmp_path, "sample_encoder_stall.json", session_id, "bench_pack_002.json")
    _copy_sample_to_session(tmp_path, "sample_axis1_fault.json", session_id, "bench_pack_003.json")

    summary = compare_latest_bench_packs_in_session(tmp_path, session_id)

    assert summary["session_id"] == session_id
    assert summary["left_capture"] == "2026-04-05T12:53:20+08:00"
    assert summary["right_capture"] == "2026-04-05T12:53:47+08:00"
    assert any(item["axis_id"] == "1" and item["changed"] for item in summary["axis_diffs"])


def test_render_session_bundle_includes_latest_change_snapshot(tmp_path: Path) -> None:
    session_id = "stub-session"
    _copy_sample_to_session(tmp_path, "sample_nominal.json", session_id, "bench_pack_001.json")
    _copy_sample_to_session(tmp_path, "sample_axis1_fault.json", session_id, "bench_pack_002.json")

    result = render_session_bundle_markdown(tmp_path, session_id)
    rendered = Path(result["output_path"]).read_text(encoding="utf-8")

    assert result["pack_count"] == 2
    assert "## Latest Change Snapshot" in rendered
    assert "Changed axes: axis1" in rendered
    assert "Transport change: False (readable -> readable)" in rendered


def test_render_pack_first_run_includes_session_fields(tmp_path: Path) -> None:
    result = render_bench_pack_markdown(
        tmp_path,
        SAMPLES_ROOT / "sample_nominal.json",
        template="first-run",
    )
    rendered = Path(result["output_path"]).read_text(encoding="utf-8")

    assert "Session ID: stub-sample-set-v1" in rendered
    assert "Session label: Stub sample set" in rendered
    assert "## Axis 0" in rendered
    assert "## Axis 1" in rendered


def test_render_pack_issue_includes_raw_evidence_block(tmp_path: Path) -> None:
    result = render_bench_pack_markdown(
        tmp_path,
        SAMPLES_ROOT / "sample_open_rpmsg_fail.json",
        template="issue",
    )
    rendered = Path(result["output_path"]).read_text(encoding="utf-8")

    assert "## Raw Evidence" in rendered
    assert '"stub_scenario": "open_rpmsg_fail"' in rendered
    assert "parsed status: ok" in rendered


def test_summarize_bench_sessions_reports_bundle_review_presence(tmp_path: Path) -> None:
    session_id = "stub-session"
    _copy_sample_to_session(tmp_path, "sample_nominal.json", session_id, "bench_pack_001.json")
    _copy_sample_to_session(tmp_path, "sample_axis1_fault.json", session_id, "bench_pack_002.json")
    render_session_bundle_markdown(tmp_path, session_id)

    summary = summarize_bench_sessions(tmp_path)

    assert summary["session_count"] == 1
    session = summary["sessions"][0]
    assert session["session_id"] == session_id
    assert session["pack_count"] == 2
    assert session["has_bundle_review"] is True
    assert session["bundle_review_path"] == f"reports/bench_packs/rendered/{session_id}_session-bundle.md"


def test_run_local_checks_without_rag(monkeypatch, tmp_path: Path) -> None:
    items = [
        BenchmarkItem(
            item_id="safety-004",
            item_type="tool_safety",
            difficulty="easy",
            tags=[],
            input_payload={},
            expected_payload={},
        ),
    ]

    monkeypatch.setattr("industrial_embedded_dev_agent.runner.load_benchmark_items", lambda path: items)
    monkeypatch.setattr(
        "industrial_embedded_dev_agent.runner._run_pytest_check",
        lambda root: {"name": "pytest", "passed": True, "details": {}},
    )

    def fake_run_benchmark(run_items, *, root, engine):
        return {
            "engine": engine,
            "total": len(run_items),
            "passed": len(run_items),
            "failed": 0,
            "pass_rate": 1.0,
            "citation_passed": len(run_items),
            "citation_pass_rate": 1.0,
        }

    monkeypatch.setattr("industrial_embedded_dev_agent.runner.run_benchmark", fake_run_benchmark)

    result = run_local_checks_with_options(tmp_path)

    assert result["passed"] is True
    assert [check["name"] for check in result["checks"]] == [
        "pytest",
        "benchmark_rules",
        "benchmark_tool_safety",
    ]


def test_run_local_checks_with_rag_filter(monkeypatch, tmp_path: Path) -> None:
    items = [
        BenchmarkItem(
            item_id="qa-001",
            item_type="knowledge_qa",
            difficulty="easy",
            tags=[],
            input_payload={},
            expected_payload={},
        ),
        BenchmarkItem(
            item_id="safety-004",
            item_type="tool_safety",
            difficulty="easy",
            tags=[],
            input_payload={},
            expected_payload={},
        ),
    ]
    calls: list[tuple[str, int]] = []

    monkeypatch.setattr("industrial_embedded_dev_agent.runner.load_benchmark_items", lambda path: items)
    monkeypatch.setattr(
        "industrial_embedded_dev_agent.runner._run_pytest_check",
        lambda root: {"name": "pytest", "passed": True, "details": {}},
    )

    def fake_run_benchmark(run_items, *, root, engine):
        calls.append((engine, len(run_items)))
        return {
            "engine": engine,
            "total": len(run_items),
            "passed": len(run_items),
            "failed": 0,
            "pass_rate": 1.0,
            "citation_passed": len(run_items),
            "citation_pass_rate": 1.0,
        }

    monkeypatch.setattr("industrial_embedded_dev_agent.runner.run_benchmark", fake_run_benchmark)

    result = run_local_checks_with_options(tmp_path, include_rag=True, rag_item_type="tool_safety")

    assert result["passed"] is True
    assert [check["name"] for check in result["checks"]] == [
        "pytest",
        "benchmark_rules",
        "benchmark_tool_safety",
        "benchmark_rag",
    ]
    assert calls == [
        ("rules", 2),
        ("tools", 1),
        ("rag", 1),
    ]


def test_run_local_checks_with_offline_samples(monkeypatch, tmp_path: Path) -> None:
    items = [
        BenchmarkItem(
            item_id="safety-004",
            item_type="tool_safety",
            difficulty="easy",
            tags=[],
            input_payload={},
            expected_payload={},
        ),
    ]

    monkeypatch.setattr("industrial_embedded_dev_agent.runner.load_benchmark_items", lambda path: items)
    monkeypatch.setattr(
        "industrial_embedded_dev_agent.runner._run_pytest_check",
        lambda root: {"name": "pytest", "passed": True, "details": {}},
    )
    monkeypatch.setattr(
        "industrial_embedded_dev_agent.runner._run_offline_stub_regression_check",
        lambda root: {"name": "offline_stub_samples", "passed": True, "details": {"pair_count": 3}},
    )

    def fake_run_benchmark(run_items, *, root, engine):
        return {
            "engine": engine,
            "total": len(run_items),
            "passed": len(run_items),
            "failed": 0,
            "pass_rate": 1.0,
            "citation_passed": len(run_items),
            "citation_pass_rate": 1.0,
        }

    monkeypatch.setattr("industrial_embedded_dev_agent.runner.run_benchmark", fake_run_benchmark)

    result = run_local_checks_with_options(tmp_path, include_offline=True)

    assert result["passed"] is True
    assert [check["name"] for check in result["checks"]] == [
        "pytest",
        "benchmark_rules",
        "benchmark_tool_safety",
        "offline_stub_samples",
    ]


def test_prepare_real_bench_package_generates_bundle(tmp_path: Path) -> None:
    result = prepare_real_bench_package(
        REPO_ROOT,
        session_id="bench-am-01",
        label="Morning bench",
        output_dir=tmp_path / "prep_bundle",
    )

    output_dir = Path(result["output_dir"])
    assert result["session_id"] == "bench-am-01"
    assert output_dir.exists()
    assert len(result["files"]) == 7

    doctor_snapshot = output_dir / "doctor_snapshot.json"
    plan_seed = output_dir / "plan_seed.json"
    index_text = (output_dir / "00_index.md").read_text(encoding="utf-8")
    readiness_text = (output_dir / "01_readiness_checklist.md").read_text(encoding="utf-8")
    review_text = (output_dir / "04_session_review.md").read_text(encoding="utf-8")
    doctor_payload = json.loads(doctor_snapshot.read_text(encoding="utf-8"))
    seed_payload = json.loads(plan_seed.read_text(encoding="utf-8"))

    assert "Session ID: bench-am-01" in index_text
    assert "Git branch:" in index_text
    assert "Git commit:" in index_text
    assert "## Current Runtime Snapshot" in index_text
    assert "## Included Files" in index_text
    assert "doctor_snapshot.json" in index_text
    assert "plan_seed.json" in index_text
    assert "tools use-real" in index_text
    assert "kickoff-real-bench" in index_text
    assert "--render-all" in index_text
    assert "Source template: real_bench_readiness_checklist.md" in readiness_text
    assert "Session label: Morning bench" in review_text
    assert doctor_payload["session_id"] == "bench-am-01"
    assert doctor_payload["label"] == "Morning bench"
    assert "doctor" in doctor_payload
    assert "git_context" in doctor_payload
    assert seed_payload["session_id"] == "bench-am-01"
    assert seed_payload["expected_tool_id"] == "SCRIPT-004"
    assert seed_payload["expected_risk_level"] == "L0_readonly"
    assert "axis0/axis1" in seed_payload["request"]
    assert "tools plan" in seed_payload["commands"]["plan"]
    assert "--session-id bench-am-01" in seed_payload["commands"]["bench_pack"]


def test_kickoff_real_bench_from_seed_plan_only(tmp_path: Path) -> None:
    prep = prepare_real_bench_package(
        REPO_ROOT,
        session_id="bench-am-01",
        label="Morning bench",
        output_dir=tmp_path / "prep_bundle",
    )

    result = kickoff_real_bench(
        REPO_ROOT,
        Path(prep["plan_seed_path"]),
        execute=False,
    )

    bench_pack = result["bench_pack"]
    assert result["session_id"] == "bench-am-01"
    assert result["execute"] is False
    assert bench_pack["session_id"] == "bench-am-01"
    assert "axis0/axis1" in bench_pack["request"]
    assert bench_pack["result"]["plan"]["tool_id"] == "SCRIPT-004"
    assert bench_pack["result"]["plan"]["risk_level"] == "L0_readonly"
    assert bench_pack["result"]["execution"]["parsed_output"]["status"] == "skipped"
    assert Path(bench_pack["saved_to"]).exists()
    archive = result["archive"]
    archive_dir = Path(archive["output_dir"])
    assert archive_dir.exists()
    assert (archive_dir / "bench_pack.json").exists()
    assert (archive_dir / "run_summary.json").exists()
    assert (archive_dir / "run_summary.md").exists()


def test_kickoff_real_bench_can_render_first_run_draft(tmp_path: Path) -> None:
    prep = prepare_real_bench_package(
        REPO_ROOT,
        session_id="bench-am-02",
        label="Morning bench render",
        output_dir=tmp_path / "prep_bundle",
    )

    result = kickoff_real_bench(
        REPO_ROOT,
        Path(prep["plan_seed_path"]),
        execute=False,
        render_first_run=True,
    )

    bench_pack = result["bench_pack"]
    rendered = result["rendered_first_run"]
    output_path = Path(rendered["output_path"])
    markdown = output_path.read_text(encoding="utf-8")

    assert result["session_id"] == "bench-am-02"
    assert rendered["template"] == "first-run"
    assert Path(bench_pack["saved_to"]).exists()
    assert output_path.exists()
    assert "Session ID: bench-am-02" in markdown
    assert "Session label: Morning bench render" in markdown
    assert "tool_id: SCRIPT-004" in markdown


def test_kickoff_real_bench_can_render_session_review_draft(tmp_path: Path) -> None:
    prep = prepare_real_bench_package(
        REPO_ROOT,
        session_id="bench-am-03",
        label="Morning bench review",
        output_dir=tmp_path / "prep_bundle",
    )

    result = kickoff_real_bench(
        REPO_ROOT,
        Path(prep["plan_seed_path"]),
        execute=False,
        render_session_review=True,
    )

    bench_pack = result["bench_pack"]
    rendered = result["rendered_session_review"]
    output_path = Path(rendered["output_path"])
    markdown = output_path.read_text(encoding="utf-8")

    assert result["session_id"] == "bench-am-03"
    assert rendered["template"] == "session-review"
    assert Path(bench_pack["saved_to"]).exists()
    assert output_path.exists()
    assert "Session ID: bench-am-03" in markdown
    assert "Session label: Morning bench review" in markdown
    assert "## Final Session Verdict" in markdown


def test_kickoff_real_bench_can_render_both_drafts(tmp_path: Path) -> None:
    prep = prepare_real_bench_package(
        REPO_ROOT,
        session_id="bench-am-04",
        label="Morning bench all",
        output_dir=tmp_path / "prep_bundle",
    )

    result = kickoff_real_bench(
        REPO_ROOT,
        Path(prep["plan_seed_path"]),
        execute=False,
        render_first_run=True,
        render_session_review=True,
    )

    first_run = result["rendered_first_run"]
    session_review = result["rendered_session_review"]
    first_run_text = Path(first_run["output_path"]).read_text(encoding="utf-8")
    session_review_text = Path(session_review["output_path"]).read_text(encoding="utf-8")

    assert first_run["template"] == "first-run"
    assert session_review["template"] == "session-review"
    assert "Session ID: bench-am-04" in first_run_text
    assert "Session ID: bench-am-04" in session_review_text
    archive = result["archive"]
    archive_dir = Path(archive["output_dir"])
    summary_payload = json.loads((archive_dir / "run_summary.json").read_text(encoding="utf-8"))
    summary_markdown = (archive_dir / "run_summary.md").read_text(encoding="utf-8")

    assert (archive_dir / "first_run.md").exists()
    assert (archive_dir / "session_review.md").exists()
    assert summary_payload["session_id"] == "bench-am-04"
    assert summary_payload["bench_pack_path"].endswith("bench_pack.json")
    assert summary_payload["first_run_path"].endswith("first_run.md")
    assert summary_payload["session_review_path"].endswith("session_review.md")
    assert "## Archived Outputs" in summary_markdown


def test_finish_real_bench_collects_closing_outputs(tmp_path: Path) -> None:
    session_id = "bench-am-06"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    prep = prepare_real_bench_package(
        REPO_ROOT,
        session_id=session_id,
        label="Morning bench finish",
        output_dir=tmp_path / "prep_bundle",
    )

    kickoff_real_bench(
        REPO_ROOT,
        Path(prep["plan_seed_path"]),
        execute=False,
        render_first_run=True,
        render_session_review=True,
    )

    result = finish_real_bench(
        REPO_ROOT,
        session_id=session_id,
        prep_dir=Path(prep["output_dir"]),
    )

    output_dir = Path(result["output_dir"])
    summary_json = output_dir / "final_summary.json"
    summary_md = output_dir / "final_summary.md"
    summary_payload = json.loads(summary_json.read_text(encoding="utf-8"))
    summary_text = summary_md.read_text(encoding="utf-8")
    candidate_dir = output_dir / "candidate_exports"
    case_candidate = candidate_dir / "case_candidate.md"
    log_candidate = candidate_dir / "log_candidate.json"
    benchmark_candidate = candidate_dir / "benchmark_candidate.json"

    assert output_dir.exists()
    assert (output_dir / "session_bundle.md").exists()
    assert (output_dir / "kickoff_run_summary.json").exists()
    assert (output_dir / "kickoff_run_summary.md").exists()
    assert summary_json.exists()
    assert summary_md.exists()
    assert summary_payload["session_id"] == session_id
    assert summary_payload["pack_count"] >= 1
    assert summary_payload["kickoff_outputs_present"] is True
    assert "## Aggregated Outputs" in summary_text
    assert candidate_dir.exists()
    assert case_candidate.exists()
    assert log_candidate.exists()
    assert benchmark_candidate.exists()
    assert result["candidate_exports"]["case_candidate"].endswith("case_candidate.md")
    assert result["candidate_exports"]["benchmark_candidate"].endswith("benchmark_candidate.json")


def test_review_finish_candidates_generates_review_summary(tmp_path: Path) -> None:
    session_id = "bench-am-07"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    prep = prepare_real_bench_package(
        REPO_ROOT,
        session_id=session_id,
        label="Morning bench review candidates",
        output_dir=tmp_path / "prep_bundle",
    )

    kickoff_real_bench(
        REPO_ROOT,
        Path(prep["plan_seed_path"]),
        execute=False,
        render_first_run=True,
        render_session_review=True,
    )
    finish_real_bench(
        REPO_ROOT,
        session_id=session_id,
        prep_dir=Path(prep["output_dir"]),
    )

    result = review_finish_candidates(
        REPO_ROOT,
        session_id=session_id,
        prep_dir=Path(prep["output_dir"]),
    )

    review_dir = Path(result["output_dir"])
    review_json = review_dir / "review_summary.json"
    review_md = review_dir / "review_summary.md"
    review_payload = json.loads(review_json.read_text(encoding="utf-8"))
    review_text = review_md.read_text(encoding="utf-8")

    assert review_dir.exists()
    assert review_json.exists()
    assert review_md.exists()
    assert review_payload["session_id"] == session_id
    assert "## Reviewer Checklist" in review_text
    assert "suggested_tag" in review_text


def test_promote_finish_candidates_copies_into_pending_area(tmp_path: Path) -> None:
    session_id = "bench-am-08"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench promote",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )

        result = promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )

        pending_root = Path(result["pending_root"])
        record_path = Path(result["promotion_record"])
        pending_jsonl = pending_root / "benchmarks" / "pending_benchmark_candidates.jsonl"
        record_payload = json.loads(record_path.read_text(encoding="utf-8"))
        pending_lines = pending_jsonl.read_text(encoding="utf-8").strip().splitlines()

        assert pending_root.exists()
        assert (pending_root / "cases" / f"{session_id}_case_candidate.md").exists()
        assert (pending_root / "logs" / f"{session_id}_log_candidate.json").exists()
        assert (pending_root / "benchmarks" / f"{session_id}_benchmark_candidate.json").exists()
        assert pending_jsonl.exists()
        assert len(pending_lines) == 1
        assert record_payload["session_id"] == session_id
    finally:
        _reset_pending_root()


def test_plan_pending_merge_generates_merge_plan(tmp_path: Path) -> None:
    session_id = "bench-am-09"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench merge plan",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )

        result = plan_pending_merge(REPO_ROOT)
        plan_dir = Path(result["output_dir"])
        plan_json = plan_dir / "merge_plan.json"
        plan_md = plan_dir / "merge_plan.md"
        plan_payload = json.loads(plan_json.read_text(encoding="utf-8"))
        plan_text = plan_md.read_text(encoding="utf-8")

        assert plan_dir.exists()
        assert plan_json.exists()
        assert plan_md.exists()
        assert len(plan_payload["case_candidates"]) >= 1
        assert len(plan_payload["benchmark_candidates"]) >= 1
        assert "## Merge Guidance" in plan_text
    finally:
        _reset_pending_root()


def test_prepare_formal_merge_assistant_generates_draft_merge_bundle(tmp_path: Path) -> None:
    session_id = "bench-am-10"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench formal merge",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )

        result = prepare_formal_merge_assistant(REPO_ROOT)
        assistant_dir = Path(result["output_dir"])
        assistant_json = assistant_dir / "formal_merge_assistant.json"
        assistant_md = assistant_dir / "formal_merge_assistant.md"
        case_bundle = assistant_dir / "materials_case_merge_candidates.md"
        benchmark_append = assistant_dir / "benchmark_append_candidates.jsonl"
        material_index_patch = assistant_dir / "material_index_patch.md"

        assistant_payload = json.loads(assistant_json.read_text(encoding="utf-8"))
        assistant_text = assistant_md.read_text(encoding="utf-8")
        benchmark_lines = benchmark_append.read_text(encoding="utf-8").strip().splitlines()

        assert assistant_dir.exists()
        assert assistant_json.exists()
        assert assistant_md.exists()
        assert case_bundle.exists()
        assert benchmark_append.exists()
        assert material_index_patch.exists()
        assert assistant_payload["counts"]["case_candidates"] >= 1
        assert assistant_payload["counts"]["benchmark_candidates"] >= 1
        assert "## Recommended Merge Order" in assistant_text
        assert len(benchmark_lines) >= 1
    finally:
        _reset_pending_root()


def test_apply_formal_merge_generates_dry_run_summary(tmp_path: Path) -> None:
    session_id = "bench-am-11"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench apply formal merge",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )

        result = apply_formal_merge(REPO_ROOT, dry_run=True)
        summary_json = Path(result["apply_formal_merge_json"])
        summary_md = Path(result["apply_formal_merge_markdown"])
        payload = json.loads(summary_json.read_text(encoding="utf-8"))
        text = summary_md.read_text(encoding="utf-8")
        benchmark_patch = Path(result["benchmark_append_patch"])
        material_index_patch = Path(result["material_index_append_patch"])
        commit_plan = Path(result["recommended_commit_split"])
        benchmark_patch_lines = benchmark_patch.read_text(encoding="utf-8").strip().splitlines()
        commit_plan_text = commit_plan.read_text(encoding="utf-8")

        assert summary_json.exists()
        assert summary_md.exists()
        assert benchmark_patch.exists()
        assert material_index_patch.exists()
        assert commit_plan.exists()
        assert payload["dry_run"] is True
        assert len(payload["planned_actions"]["benchmark_appends"]) >= 1
        assert "## Benchmark Appends" in text
        assert "Dry-run only" in text
        assert "## Recommended Commit Split" in text
        assert len(benchmark_patch_lines) >= 1
        assert "formal-benchmark-appends" in commit_plan_text
    finally:
        _reset_pending_root()


def test_apply_formal_merge_execute_writes_staging_only(tmp_path: Path) -> None:
    session_id = "bench-am-12"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    benchmark_target = REPO_ROOT / "data" / "benchmark" / "benchmark_v1.jsonl"
    benchmark_before = benchmark_target.read_text(encoding="utf-8")

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench apply execute staging",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )

        result = apply_formal_merge(REPO_ROOT, dry_run=False)
        staging_root = Path(result["staging_root"])
        staged_files = result["staged_files"]

        assert result["dry_run"] is False
        assert staging_root.exists()
        assert (staging_root / "data" / "materials" / "materials_case_merge_candidates.md").exists()
        assert (staging_root / "data" / "materials" / "material_index_append_patch.md").exists()
        assert (staging_root / "data" / "benchmark" / "benchmark_append_patch.jsonl").exists()
        assert (staging_root / "staging_summary.json").exists()
        assert "benchmark_append_patch" in staged_files
        assert benchmark_target.read_text(encoding="utf-8") == benchmark_before
    finally:
        _reset_pending_root()


def test_canonical_merge_preflight_reports_ready_staging_bundle(tmp_path: Path) -> None:
    session_id = "bench-am-13"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench canonical preflight",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        apply_formal_merge(REPO_ROOT, dry_run=False)

        result = canonical_merge_preflight(REPO_ROOT)
        summary_json = Path(result["canonical_merge_preflight_json"])
        summary_md = Path(result["canonical_merge_preflight_markdown"])
        payload = json.loads(summary_json.read_text(encoding="utf-8"))
        text = summary_md.read_text(encoding="utf-8")
        checks = {item["name"]: item for item in payload["checks"]}

        assert summary_json.exists()
        assert summary_md.exists()
        assert payload["passed"] is True
        assert checks["staging_bundle_ready"]["passed"] is True
        assert checks["benchmark_duplicate_ids"]["passed"] is True
        assert "## Checks" in text
    finally:
        _reset_pending_root()


def test_canonical_patch_helper_generates_manifest_and_patch_bundle(tmp_path: Path) -> None:
    session_id = "bench-am-14"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench canonical patch helper",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        apply_formal_merge(REPO_ROOT, dry_run=False)

        result = canonical_patch_helper(REPO_ROOT)
        manifest_json = Path(result["canonical_patch_manifest_json"])
        manifest_md = Path(result["canonical_patch_manifest_markdown"])
        benchmark_patch = Path(result["benchmark_append_patch"])
        material_index_patch = Path(result["material_index_append_patch"])
        materials_case_candidates = Path(result["materials_case_candidates"])
        payload = json.loads(manifest_json.read_text(encoding="utf-8"))
        text = manifest_md.read_text(encoding="utf-8")

        assert manifest_json.exists()
        assert manifest_md.exists()
        assert benchmark_patch.exists()
        assert material_index_patch.exists()
        assert materials_case_candidates.exists()
        assert payload["patch_files"]["benchmark_append_patch"].endswith("benchmark_v1.append.jsonl")
        assert "## Patch Files" in text
    finally:
        _reset_pending_root()


def test_canonical_merge_preview_bundle_generates_preview_files(tmp_path: Path) -> None:
    session_id = "bench-am-15"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench canonical merge preview",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        apply_formal_merge(REPO_ROOT, dry_run=False)

        result = canonical_merge_preview_bundle(REPO_ROOT)
        manifest_json = Path(result["canonical_merge_preview_manifest_json"])
        manifest_md = Path(result["canonical_merge_preview_manifest_markdown"])
        benchmark_preview = Path(result["benchmark_preview"])
        material_index_preview = Path(result["material_index_preview"])
        materials_case_preview = Path(result["materials_case_preview"])
        payload = json.loads(manifest_json.read_text(encoding="utf-8"))
        benchmark_preview_text = benchmark_preview.read_text(encoding="utf-8")
        material_index_preview_text = material_index_preview.read_text(encoding="utf-8")

        assert manifest_json.exists()
        assert manifest_md.exists()
        assert benchmark_preview.exists()
        assert material_index_preview.exists()
        assert materials_case_preview.exists()
        assert payload["preview_files"]["benchmark_preview"].endswith("benchmark_v1.preview.jsonl")
        assert "candidate-bench-am-15" in benchmark_preview_text
        assert "## Pending Preview Entries" in material_index_preview_text
    finally:
        _reset_pending_root()


def test_canonical_merge_report_collects_preflight_patch_and_preview_outputs(tmp_path: Path) -> None:
    session_id = "bench-am-16"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench canonical merge report",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        apply_formal_merge(REPO_ROOT, dry_run=False)

        result = canonical_merge_report(REPO_ROOT)
        report_json = Path(result["canonical_merge_report_json"])
        report_md = Path(result["canonical_merge_report_markdown"])
        payload = json.loads(report_json.read_text(encoding="utf-8"))
        text = report_md.read_text(encoding="utf-8")

        assert report_json.exists()
        assert report_md.exists()
        assert payload["preflight"]["markdown"].endswith("canonical_merge_preflight.md")
        assert payload["patch_bundle"]["manifest_markdown"].endswith("canonical_patch_manifest.md")
        assert payload["preview_bundle"]["manifest_markdown"].endswith("canonical_merge_preview_manifest.md")
        assert "## Preflight" in text
        assert "## Patch Bundle" in text
        assert "## Preview Bundle" in text
        assert "## Recommended Review Order" in text
    finally:
        _reset_pending_root()


def test_canonical_merge_checklist_reports_manual_review_readiness(tmp_path: Path) -> None:
    session_id = "bench-am-17"
    session_dir = REPO_ROOT / "reports" / "bench_packs" / "sessions" / session_id
    if session_dir.exists():
        shutil.rmtree(session_dir)

    _reset_pending_root()
    try:
        prep = prepare_real_bench_package(
            REPO_ROOT,
            session_id=session_id,
            label="Morning bench canonical merge checklist",
            output_dir=tmp_path / "prep_bundle",
        )

        kickoff_real_bench(
            REPO_ROOT,
            Path(prep["plan_seed_path"]),
            execute=False,
            render_first_run=True,
            render_session_review=True,
        )
        finish_real_bench(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        review_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        promote_finish_candidates(
            REPO_ROOT,
            session_id=session_id,
            prep_dir=Path(prep["output_dir"]),
        )
        apply_formal_merge(REPO_ROOT, dry_run=False)

        result = canonical_merge_checklist(REPO_ROOT)
        checklist_json = Path(result["canonical_merge_checklist_json"])
        checklist_md = Path(result["canonical_merge_checklist_markdown"])
        payload = json.loads(checklist_json.read_text(encoding="utf-8"))
        text = checklist_md.read_text(encoding="utf-8")

        assert checklist_json.exists()
        assert checklist_md.exists()
        assert result["ready_for_manual_canonical_merge_review"] is True
        assert payload["ready_for_manual_canonical_merge_review"] is True
        assert "## Checklist Items" in text
        assert "## Next Steps" in text
    finally:
        _reset_pending_root()
