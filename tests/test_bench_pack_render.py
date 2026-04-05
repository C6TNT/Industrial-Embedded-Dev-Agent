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
from industrial_embedded_dev_agent.tools import kickoff_real_bench, prepare_real_bench_package


REPO_ROOT = Path(__file__).resolve().parents[1]
SAMPLES_ROOT = REPO_ROOT / "data" / "examples" / "stub_bench_packs"


def _copy_sample_to_session(tmp_path: Path, sample_name: str, session_id: str, target_name: str) -> Path:
    session_dir = tmp_path / "reports" / "bench_packs" / "sessions" / session_id
    session_dir.mkdir(parents=True, exist_ok=True)
    destination = session_dir / target_name
    shutil.copyfile(SAMPLES_ROOT / sample_name, destination)
    return destination


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
    assert "doctor_snapshot.json" in index_text
    assert "plan_seed.json" in index_text
    assert "tools use-real" in index_text
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
