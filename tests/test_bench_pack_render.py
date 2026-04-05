from __future__ import annotations

import shutil
from pathlib import Path

from industrial_embedded_dev_agent.bench_pack_render import (
    compare_bench_packs,
    compare_latest_bench_packs_in_session,
    render_session_bundle_markdown,
    summarize_bench_sessions,
)


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
