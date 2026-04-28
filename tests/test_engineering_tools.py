from __future__ import annotations

import json
from pathlib import Path

from industrial_embedded_dev_agent.engineering import (
    audit_hardware_action,
    draft_material_fact,
    draft_project_fact,
    gsd_status,
    import_real_report,
    project_status,
    run_gsd_offline,
    scan_secrets,
    summarize_fake_regression,
    write_regression_overview,
)
from industrial_embedded_dev_agent.material_workspace import (
    build_material_inventory,
    build_material_status,
    classify_material_tool,
    plan_material_tool,
    run_material_tool,
    search_material_workspace,
    summarize_material_tools,
)
import industrial_embedded_dev_agent.engineering as engineering_module


REPO_ROOT = Path(__file__).resolve().parents[1]


def test_secret_scan_detects_content_secret_in_plain_directory(tmp_path: Path) -> None:
    (tmp_path / "safe.md").write_text("normal engineering note\n", encoding="utf-8")
    secret_text = "api_" + "key = " + '"abcdefghijklmnop"' + "\n"
    (tmp_path / "leak.txt").write_text(secret_text, encoding="utf-8")

    result = scan_secrets(tmp_path)

    assert result["passed"] is False
    assert result["sensitive_filename_hits"] == []
    assert result["content_secret_hits"][0]["file"] == "leak.txt"
    assert result["content_secret_hits"][0]["label"] == "generic_assignment"


def test_secret_scan_current_repository_is_clean() -> None:
    result = scan_secrets(REPO_ROOT)

    assert result["passed"] is True
    assert result["content_secret_hits"] == []


def test_draft_project_fact_writes_reviewable_artifacts(tmp_path: Path) -> None:
    result = draft_project_fact(
        REPO_ROOT,
        "robot6 axis1/slave2 uses 12/28 while other axes use 19/13.",
        title="axis1 variant",
        source="unit-test",
        output_dir=tmp_path / "fact",
    )

    output_dir = Path(result["output_dir"])
    payload = json.loads((output_dir / "fact_draft.json").read_text(encoding="utf-8"))

    assert output_dir.exists()
    assert (output_dir / "fact_draft.md").exists()
    assert (output_dir / "benchmark_candidate.json").exists()
    assert payload["title"] == "axis1 variant"
    assert "benchmark_candidate" in payload
    assert any("data/benchmark" in item for item in payload["suggested_updates"])


def test_import_real_report_generates_log_and_replay_drafts(tmp_path: Path) -> None:
    report = tmp_path / "real_report.json"
    report.write_text(
        json.dumps(
            {
                "driver": "inovance_sv660n",
                "axis": 1,
                "slave": 2,
                "ob": 12,
                "ib": 28,
                "status_word": 0x1234,
                "error_code": 0,
                "actual_position_before": 100,
                "actual_position_after": 300,
                "actual_vel": 0,
                "passed": True,
            },
            ensure_ascii=False,
        ),
        encoding="utf-8",
    )

    result = import_real_report(REPO_ROOT, report, output_dir=tmp_path / "imported")
    summary = json.loads(Path(result["summary_json"]).read_text(encoding="utf-8"))

    assert Path(result["log_entry_draft"]).exists()
    assert Path(result["replay_scenario_draft"]).exists()
    assert summary["extracted"]["axis"] == 1
    assert summary["replay_scenario_draft"]["slave"] == 2
    assert summary["log_entry_draft"]["suggested_tag"] == "verification_tooling"


def test_summarize_fake_regression_counts_pass_and_fail(tmp_path: Path) -> None:
    matrix = tmp_path / "matrix.json"
    matrix.write_text(
        json.dumps(
            {
                "results": [
                    {"scenario": "inovance_normal", "passed": True},
                    {"scenario": "gate_locked", "passed": False, "fail_reason": "gate locked"},
                ]
            },
            ensure_ascii=False,
        ),
        encoding="utf-8",
    )

    result = summarize_fake_regression(REPO_ROOT, matrix, output_dir=tmp_path / "summary")
    summary = json.loads(Path(result["summary_json"]).read_text(encoding="utf-8"))

    assert summary["total_records"] == 2
    assert summary["passed"] == 1
    assert summary["failed"] == 1
    assert Path(result["summary_markdown"]).exists()


def test_audit_hardware_action_separates_motion_boundary() -> None:
    result = audit_hardware_action(REPO_ROOT, "unlock 0x41F1 and move robot axis5")

    assert result["boundary"] == "hardware_window_required"
    assert result["requires_manual_confirmation"] is True
    assert "output_gate_required" in result["requirements"]
    assert "robot_motion_required" in result["requirements"]
    assert result["should_execute_automatically"] is False
    assert result["diagnosis"]["risk_level"] == "L2_high_risk_exec"


def test_audit_hardware_action_allows_board_diagnostic_dry_run_only() -> None:
    dry_run = audit_hardware_action(REPO_ROOT, "spindle tools board-status")
    execute = audit_hardware_action(REPO_ROOT, "spindle tools board-status --execute")

    assert dry_run["boundary"] == "offline_ok"
    assert dry_run["should_execute_automatically"] is True
    assert execute["boundary"] == "hardware_window_required"
    assert execute["requires_manual_confirmation"] is True
    assert "board_required" in execute["requirements"]
    assert any("board-only read window" in item for item in execute["required_conditions"])


def test_project_status_and_regression_overview_are_machine_readable(tmp_path: Path) -> None:
    status = project_status(REPO_ROOT)
    overview = write_regression_overview(
        REPO_ROOT,
        {"passed": True, "checks": [{"name": "pytest", "passed": True}]},
        output_dir=tmp_path,
    )

    assert status["baseline"]["hardware_required_for_agent_development"] is False
    assert status["benchmark"]["total"] >= 1
    assert "material_workspace" in status
    assert status["hardware_boundary"]["offline_ok"]
    assert Path(overview["regression_overview_json"]).exists()
    assert Path(overview["regression_overview_markdown"]).exists()


def test_tool_plan_blocks_hardware_tokens_even_with_fake_harness_context() -> None:
    from industrial_embedded_dev_agent.tools import plan_tool_request

    plan = plan_tool_request(
        REPO_ROOT,
        "After the offline fake harness passes, also SSH to the board, start-bus, unlock 0x41F1, and move robot axis5.",
    )

    assert plan.should_refuse is True
    assert plan.allowed_to_execute is False
    assert plan.risk_level == "L2_high_risk_exec"
    assert "start-bus" in plan.evidence
    assert "0x41f1" in plan.evidence
    assert "move robot" in plan.evidence


def test_gsd_status_reports_planning_boundary() -> None:
    result = gsd_status(REPO_ROOT)

    assert "planning_ready" in result
    assert "automation_scope" in result
    assert "0x86 control word" in result["automation_scope"]["blocked"]
    assert "pytest and benchmark regression" in result["automation_scope"]["allowed"]


def test_gsd_offline_run_writes_report_and_preserves_boundary(tmp_path: Path, monkeypatch) -> None:
    state = tmp_path / ".planning" / "STATE.md"
    state.parent.mkdir(parents=True)
    state.write_text("# GSD State\n", encoding="utf-8")
    (tmp_path / ".planning" / "PROJECT.md").write_text("# Project\n", encoding="utf-8")
    (tmp_path / ".planning" / "REQUIREMENTS.md").write_text("# Requirements\n", encoding="utf-8")
    (tmp_path / ".planning" / "ROADMAP.md").write_text("# Roadmap\n", encoding="utf-8")
    (tmp_path / ".planning" / "GSD_BOUNDARY.md").write_text("# Boundary\n", encoding="utf-8")
    (tmp_path / ".planning" / "config.json").write_text('{"safety": {"autonomous_scope": "offline_ok_only"}}', encoding="utf-8")

    monkeypatch.setattr(
        engineering_module,
        "_run_guarded_local_check",
        lambda root: {"passed": True, "checks": [{"name": "pytest", "passed": True}]},
    )
    monkeypatch.setattr(
        engineering_module,
        "run_pre_push_check",
        lambda root, include_history=False, run_checks=False: {"passed": True, "checks": []},
    )

    result = run_gsd_offline(tmp_path)

    assert result["passed"] is True
    assert result["report"]["json"].endswith(".json")
    assert Path(result["report"]["markdown"]).exists()
    assert result["state_update"]["updated"] is True
    assert "robot motion" in result["blocked_autonomous_scope"]
    assert "Last Offline GSD Run" in state.read_text(encoding="utf-8")


def test_material_inventory_search_and_tool_risk(tmp_path: Path) -> None:
    workspace = tmp_path / "materials"
    tools_dir = workspace / "mix_protocol" / "tools"
    docs_dir = workspace / "mix_protocol" / "docs"
    xml_dir = workspace / "assets" / "xml"
    tools_dir.mkdir(parents=True)
    docs_dir.mkdir(parents=True)
    xml_dir.mkdir(parents=True)
    (docs_dir / "dynamic_profile_test_chain.md").write_text("0x41F1 gate protects dynamic output.\n", encoding="utf-8")
    (xml_dir / "drive.xml").write_text("<Vendor>inovance</Vendor>\n", encoding="utf-8")
    (tools_dir / "run_offline_profile_regression.py").write_text("print('offline')\n", encoding="utf-8")
    (tools_dir / "robot_axis_dynamic_position_probe.py").write_text("print('motion')\n", encoding="utf-8")
    (tools_dir / "m7_hot_reload.py").write_text("print('remoteproc')\n", encoding="utf-8")

    inventory = build_material_inventory(REPO_ROOT, material_root=workspace)
    search = search_material_workspace(REPO_ROOT, "0x41F1", scope="docs", material_root=workspace)
    tools = summarize_material_tools(REPO_ROOT, material_root=workspace)

    assert inventory["exists"] is True
    assert inventory["area_counts"]["project_docs"] == 1
    assert inventory["area_counts"]["xml_esi"] == 1
    assert search["hit_count"] == 1
    assert search["hits"][0]["path"].endswith("dynamic_profile_test_chain.md")
    assert tools["risk_counts"]["offline_ok"] == 1
    assert tools["risk_counts"]["robot_motion_required"] == 1
    assert tools["risk_counts"]["firmware_required"] == 1
    assert classify_material_tool("mix_protocol/tools/a53_send_ec_profile.cpp").risk == "board_required"


def test_material_status_summarizes_workspace_and_boundaries(tmp_path: Path) -> None:
    workspace = tmp_path / "materials"
    tools_dir = workspace / "mix_protocol" / "tools"
    docs_dir = workspace / "mix_protocol" / "docs"
    tools_dir.mkdir(parents=True)
    docs_dir.mkdir(parents=True)
    (docs_dir / "robot6.md").write_text("robot6 position baseline uses 0x41F1 gate.\n", encoding="utf-8")
    (tools_dir / "run_static_profile_tests.py").write_text("print('offline')\n", encoding="utf-8")
    (tools_dir / "a53_send_ec_profile.cpp").write_text("// board-facing\n", encoding="utf-8")

    status = build_material_status(REPO_ROOT, material_root=workspace)

    assert status["exists"] is True
    assert status["status"] == "ready_for_offline_search"
    assert status["offline_tool_count"] == 1
    assert status["blocked_tool_count"] == 1
    assert status["tool_risk_counts"]["board_required"] == 1
    assert "project_docs" in status["top_areas"]
    assert status["hardware_boundary"]["autonomous_execution"] == "offline_ok only"


def test_material_tool_plan_allows_only_offline_tools(tmp_path: Path) -> None:
    workspace = tmp_path / "materials"
    tools_dir = workspace / "mix_protocol" / "tools"
    tools_dir.mkdir(parents=True)
    (tools_dir / "run_static_profile_tests.py").write_text("print('offline')\n", encoding="utf-8")
    (tools_dir / "driver_motor_lib_verify.py").write_text("print('motion')\n", encoding="utf-8")

    offline = plan_material_tool(REPO_ROOT, "mix_protocol/tools/run_static_profile_tests.py", material_root=workspace)
    motion = plan_material_tool(REPO_ROOT, "mix_protocol/tools/driver_motor_lib_verify.py", material_root=workspace)

    assert offline["allowed_to_execute"] is True
    assert offline["command"] == ["python", "mix_protocol/tools/run_static_profile_tests.py"]
    assert motion["allowed_to_execute"] is False
    assert motion["requires_manual_confirmation"] is True
    assert motion["tool"]["risk"] == "robot_motion_required"


def test_material_run_tool_executes_only_offline_tools(tmp_path: Path) -> None:
    workspace = tmp_path / "materials"
    tools_dir = workspace / "mix_protocol" / "tools"
    tools_dir.mkdir(parents=True)
    (tools_dir / "run_static_profile_tests.py").write_text("print('offline ok')\n", encoding="utf-8")
    (tools_dir / "robot_axis_dynamic_position_probe.py").write_text("print('motion')\n", encoding="utf-8")

    offline = run_material_tool(
        REPO_ROOT,
        "mix_protocol/tools/run_static_profile_tests.py",
        material_root=workspace,
    )
    motion = run_material_tool(
        REPO_ROOT,
        "mix_protocol/tools/robot_axis_dynamic_position_probe.py",
        material_root=workspace,
    )

    assert offline["executed"] is True
    assert offline["passed"] is True
    assert "offline ok" in offline["stdout"]
    assert motion["executed"] is False
    assert motion["reason"] == "tool_not_offline_ok"
    assert motion["plan"]["tool"]["risk"] == "robot_motion_required"


def test_draft_material_fact_records_source_check(tmp_path: Path) -> None:
    workspace = tmp_path / "materials"
    source = workspace / "mix_protocol" / "docs" / "note.md"
    source.parent.mkdir(parents=True)
    source.write_text("axis1 slave2 uses 12/28 variant.\n", encoding="utf-8")

    result = draft_material_fact(
        REPO_ROOT,
        "robot6 axis1/slave2 uses the 12/28 position variant.",
        source_path="mix_protocol/docs/note.md",
        title="axis1 variant source check",
        material_root=workspace,
        output_dir=tmp_path / "draft",
    )
    source_check = json.loads(Path(result["source_check"]).read_text(encoding="utf-8"))

    assert result["source_exists"] is True
    assert source_check["source_exists"] is True
    assert source_check["source_path"] == "mix_protocol/docs/note.md"
    assert Path(result["fact_draft_json"]).exists()
