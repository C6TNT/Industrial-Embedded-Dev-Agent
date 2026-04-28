from __future__ import annotations

import json
import subprocess
from pathlib import Path

from industrial_embedded_dev_agent.board_diagnostics import (
    board_report,
    board_status,
    ethercat_query_readonly,
    m7_health,
    rpmsg_health,
)


REPO_ROOT = Path(__file__).resolve().parents[1]


def test_board_status_defaults_to_dry_run() -> None:
    result = board_status()

    assert result["executed"] is False
    assert result["passed"] is None
    assert result["target"]["host"] == "192.168.3.33"
    assert result["command"][0] == "ssh"
    assert "--start-bus" not in result["remote_script"]
    assert result["diagnosis"]["status"] == "dry_run"


def test_query_readonly_script_blocks_control_tokens() -> None:
    result = ethercat_query_readonly()
    script = result["remote_script"].lower()

    assert "--query" in script
    assert "start-bus" not in script
    assert "stop-bus" not in script
    assert "0x86" not in script
    assert "0x41f1" not in script
    assert result["safety"]["passed"] is True


def test_board_report_dry_run_writes_json_and_markdown(tmp_path: Path) -> None:
    result = board_report(REPO_ROOT, output_dir=tmp_path)

    assert result["mode"] == "dry_run"
    assert result["passed"] is None
    assert Path(result["report"]["json"]).exists()
    assert Path(result["report"]["markdown"]).exists()
    payload = json.loads(Path(result["report"]["json"]).read_text(encoding="utf-8"))
    assert payload["risk_boundary"]["default_mode"] == "dry_run"
    assert len(payload["checks"]) == 4


def test_board_status_execute_success(monkeypatch) -> None:
    def fake_run(*args, **kwargs):
        return subprocess.CompletedProcess(
            args=args[0],
            returncode=0,
            stdout=(
                "SECTION board_status\n"
                "CHECK home_dir=present\n"
                "CHECK a53_send_ec_profile=present\n"
                "CHECK rpmsg_auto_open=present\n"
            ),
            stderr="",
        )

    monkeypatch.setattr(subprocess, "run", fake_run)
    result = board_status(execute=True)

    assert result["executed"] is True
    assert result["passed"] is True
    assert result["diagnosis"]["status"] == "ok"


def test_rpmsg_health_reports_missing_device(monkeypatch) -> None:
    def fake_run(*args, **kwargs):
        return subprocess.CompletedProcess(args=args[0], returncode=0, stdout="NO_RPMSG_DEVICE\n", stderr="")

    monkeypatch.setattr(subprocess, "run", fake_run)
    result = rpmsg_health(execute=True)

    assert result["passed"] is False
    assert result["diagnosis"]["status"] == "rpmsg_device_missing"


def test_m7_health_reports_remoteproc_failure(monkeypatch) -> None:
    def fake_run(*args, **kwargs):
        return subprocess.CompletedProcess(args=args[0], returncode=0, stdout="remoteproc boot failed: bad phdr\n", stderr="")

    monkeypatch.setattr(subprocess, "run", fake_run)
    result = m7_health(execute=True)

    assert result["passed"] is False
    assert result["diagnosis"]["status"] == "m7_remoteproc_error"


def test_query_readonly_success(monkeypatch) -> None:
    def fake_run(*args, **kwargs):
        return subprocess.CompletedProcess(
            args=args[0],
            returncode=0,
            stdout="PROFILE_STATUS loaded=1 applied=1 slaves=6 inOP=1 task=1 strategy=3\n",
            stderr="",
        )

    monkeypatch.setattr(subprocess, "run", fake_run)
    result = ethercat_query_readonly(execute=True)

    assert result["passed"] is True
    assert result["diagnosis"]["status"] == "ok"


def test_ssh_failure_is_attributed(monkeypatch) -> None:
    def fake_run(*args, **kwargs):
        return subprocess.CompletedProcess(args=args[0], returncode=255, stdout="", stderr="ssh: connect to host 192.168.3.33 port 22: Connection timed out")

    monkeypatch.setattr(subprocess, "run", fake_run)
    result = board_status(execute=True)

    assert result["passed"] is False
    assert result["diagnosis"]["status"] == "ssh_unreachable"

