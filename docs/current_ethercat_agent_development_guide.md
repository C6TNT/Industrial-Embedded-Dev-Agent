# Current EtherCAT Agent Development Guide

## 1. Purpose

This Agent is currently aligned to the EtherCAT Dynamic Profile project, not the earlier dual-drive CANopen baseline.

Its job is to help with offline engineering work before the real board or robot is available:

- explain the A53 / RPMsg / M7 / EtherCAT dynamic profile chain;
- analyze profile, PDO, robot6 topology, runtime takeover, M7 deploy, and fake harness issues;
- enforce tool safety boundaries around robot motion, IO output, firmware loading, `0x86`, and `0x41F1`;
- keep benchmark, RAG chunks, and material indexes synchronized with real project evidence.

## 2. Active Engineering Baseline

Current control chain:

1. A53 parses XML/ESI.
2. A53 generates JSON profile/topology.
3. A53 sends profile/query/start/stop messages through RPMsg.
4. M7 applies the EtherCAT profile.
5. M7 performs PDO mapping, pack/unpack, and runtime axis publication.

Key runtime facts:

- Dynamic parameter area: `0x4100 + logical_axis`.
- Dynamic output gate: `0x41F1`.
- Gate rule: dynamic control writes must be blocked before the gate is explicitly unlocked.

Single-drive baseline:

| Driver | Layout | ob | ib |
|---|---:|---:|---:|
| Inovance SV660N | `fixed_layout` | 7 | 13 |
| Kinco FD | `remap` | 7 | 13 |
| YAKO MS | `remap` | 7 | 13 |

Robot6 baseline:

| Axis group | Layout |
|---|---|
| axis1 / slave2 | `12/28` position variant |
| axis0, axis2, axis3, axis4, axis5 | `19/13` baseline |

Validated robot motion baseline is position mode:

- single-axis;
- dual-axis;
- triple-axis;
- six-axis `+200 dec`.

## 3. What Can Be Developed Without Hardware

These tasks do not require a board, drives, robot, SSH, or EtherCAT cable:

- update `data/materials/material_index_v1.md`;
- update `data/materials/current_ethercat_dynamic_profile_project_v1.md`;
- update benchmark JSONL files;
- tune `src/industrial_embedded_dev_agent/analysis.py`;
- tune `src/industrial_embedded_dev_agent/rag.py`;
- tune guarded tool planning in `src/industrial_embedded_dev_agent/tools.py`;
- rebuild `data/chunks/doc_chunks_v1.jsonl`;
- run pytest, rules benchmark, RAG benchmark, and offline stub checks;
- add fake harness/replay/XML regression results as training evidence.

Recommended baseline command:

```powershell
python -m industrial_embedded_dev_agent tools project-baseline
```

## 4. What Requires Hardware

Stop before these actions unless a real hardware window is available and manually confirmed:

- real `a53_send_ec_profile --start-bus`;
- real `a53_send_ec_profile --stop-bus`;
- real RPMsg / EtherCAT query against the board;
- M7 firmware deployment or remoteproc validation;
- sending `0x86`;
- unlocking `0x41F1`;
- robot motion;
- IO, welding, homing, limit, or output switching.

## 5. Offline Development Loop

Use this loop after every material, rule, RAG, or tool-safety change:

```powershell
python -m industrial_embedded_dev_agent chunks build
python -m industrial_embedded_dev_agent benchmark run --engine rules
python -m industrial_embedded_dev_agent benchmark run --engine rag --type knowledge_qa
python -m industrial_embedded_dev_agent benchmark run --engine rag --type tool_safety
python -m industrial_embedded_dev_agent check
```

For a fuller local gate:

```powershell
python -m industrial_embedded_dev_agent check --include-offline --include-rag --rag-type tool_safety
```

## 5.1 2026-04-28 Offline Acceptance Baseline

The current offline acceptance baseline is split across the standalone fake
harness and the real `huichuan-robot-runtime/mix_protocol` worktree. The
public-facing evidence is summarized in
`data/materials/offline_acceptance_evidence_2026_04_28.md`.

Standalone `ethercat-fake-harness` verification:

- `python -m pytest tests -q`: 22 passed.
- `python tools\fake_ecat_harness\run_offline_acceptance.py`: PASS, 5/5 steps.
- fixture refresh dry-run: 40 planned, 0 copied.
- profile schema drift: 5 documents, 10 profiles, 0 errors.
- XML batch regression: `schema_version=2`, `batch_type=xml_profile_regression`,
  3 cases passed, 0 failed.
- replay batch regression: `schema_version=2`, `batch_type=real_report_replay`,
  15 cases passed, 0 failed.

Real `huichuan-robot-runtime/mix_protocol` offline verification after syncing
the fake harness report contract:

- `python tools\run_static_profile_tests.py`: `RESULT PASS passed=16 total=16`.
- `python tools\fake_ecat_harness\run_offline_acceptance.py`: PASS, 5/5 steps.
- pytest inside acceptance: 29 passed.
- fixture refresh dry-run inside acceptance: 40 noop, 0 copied.
- profile schema drift: 5 documents, 10 profiles, 0 errors.
- XML batch regression: `schema_version=2`, 3 cases passed, 0 failed.
- replay batch regression: `schema_version=2`, 15 cases passed, 0 failed.

The shared batch report contract now uses these stable fields:

- `schema`
- `schema_version`
- `batch_type`
- `generated_at`
- `root`
- `run_dir`
- `result`
- `totals`
- `failed_cases`
- `cases`

Each case keeps legacy flat fields and also includes `inputs`, `artifacts`, and
`steps`. This makes XML and replay reports easier to compare while preserving
older consumers.

Fixture refresh remains `offline_ok` only when it copies sanitized local
fixtures and runs local validation. It must not SSH, scp, reboot, reload
remoteproc, start or stop the real bus, send `0x86`, unlock `0x41F1`, write IO,
take over outputs, or move the robot.

The latest acceptance evidence supports public Agent answers about the green
offline state. It does not authorize board, bus, output gate, IO, firmware, or
robot-motion actions.

For GitHub demos, use `docs/demo_offline_acceptance_qa.md` as the short public
question script. It points to CUR-012/CUR-008/CUR-011 and keeps the answer shape
focused on acceptance counts plus the `offline_ok` boundary.

## 6. Expected Safety Behavior

Allowed by default:

- documentation and knowledge QA;
- static profile/PDO/topology analysis;
- fake harness and replay batch summary;
- read-only planning;
- offline benchmark and pytest.

Blocked by default:

- `0x86` control word execution;
- `0x41F1` output gate unlock;
- robot movement;
- IO output;
- firmware flashing or remoteproc reload;
- real start/stop bus actions without manual confirmation.

## 7. Maintenance Notes

When adding a new project fact:

1. Update the material index or current baseline document.
2. Add at least one benchmark item when the fact affects expected Agent behavior.
3. Rebuild chunks.
4. Run local checks.
5. Keep historical CANopen data as archive context only unless the current project explicitly needs it.
