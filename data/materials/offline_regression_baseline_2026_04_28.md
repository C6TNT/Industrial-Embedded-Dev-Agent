# Offline Regression Baseline 2026-04-28

This material records the verified offline state after syncing the standalone
EtherCAT fake harness improvements back into the real Huichuan runtime project.

## Scope

The work is `offline_ok`. It covers local Python tests, profile validation,
fake EtherCAT harness regression, XML profile batch regression, replay batch
regression, report format cleanup, and fixture refresh rules.

It does not approve or execute SSH, scp, reboot, remoteproc reload, real
start-bus, real stop-bus, `0x86`, `0x41F1`, takeover, IO output, or robot
motion.

## Standalone Fake Harness

Project: `ethercat-fake-harness`

Verified commands:

- `python -m pytest tests -q`: 17 passed.
- `python tools\fake_ecat_harness\run_xml_profile_batch_regression.py`: PASS.
- `python tools\fake_ecat_harness\run_replay_batch.py`: PASS.

Batch summary facts:

- XML batch uses `schema_version=2`.
- XML batch has `batch_type=xml_profile_regression`.
- XML batch passed 3 cases and failed 0 cases.
- replay batch uses `schema_version=2`.
- replay batch has `batch_type=real_report_replay`.
- replay batch passed 15 cases and failed 0 cases.

## Real Huichuan Runtime Offline Sync

Project: `huichuan-robot-runtime/mix_protocol`

Synced items:

- profile boundary pytest coverage;
- shared XML/replay batch report helper;
- schema version 2 batch summary fields;
- standalone-compatible XML path fallback;
- offline fake harness sync rules.

Verified commands:

- `python -m pytest tests -q`: 24 passed.
- `python tools\run_static_profile_tests.py`: `RESULT PASS passed=16 total=16`.
- `python tools\fake_ecat_harness\run_xml_profile_batch_regression.py`: PASS.
- `python tools\fake_ecat_harness\run_replay_batch.py`: PASS.

Batch summary facts:

- XML batch uses `schema_version=2` and passed 3/3 cases.
- replay batch uses `schema_version=2` and passed 15/15 cases.
- Both summaries reported `totals.failed=0`.

## Batch Report Contract

XML and replay batch summaries share these stable top-level fields:

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

Each case keeps legacy flat fields and also includes:

- `inputs`
- `artifacts`
- `steps`

## Fixture Refresh Rule

Refreshing fixtures is `offline_ok` only when it copies sanitized local XML,
profile, board report, topology, or replay report fixtures and then runs the
offline verification commands.

If the task requires a board, bus state change, output gate unlock, IO output,
robot movement, firmware reload, or real deployment, it crosses the offline
boundary and must stop for explicit hardware classification and approval.
