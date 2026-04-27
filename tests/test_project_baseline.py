from __future__ import annotations

from pathlib import Path

from industrial_embedded_dev_agent.tools import current_project_baseline


REPO_ROOT = Path(__file__).resolve().parents[1]


def test_current_project_baseline_describes_ethercat_dynamic_profile() -> None:
    payload = current_project_baseline(REPO_ROOT)

    assert payload["hardware_required_for_agent_development"] is False
    assert "EtherCAT Dynamic Profile" in payload["project"]
    assert payload["dynamic_runtime_facts"]["parameter_base"] == "0x4100 + logical_axis"
    assert payload["dynamic_runtime_facts"]["output_gate"] == "0x41F1"
    assert payload["robot6_baseline"]["axis1_slave2"] == "12/28 position variant"
    assert all(payload["canonical_files_present"].values())
