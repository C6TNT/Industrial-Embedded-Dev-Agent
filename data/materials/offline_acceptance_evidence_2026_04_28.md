# Offline Acceptance Evidence 2026-04-28

This material records the public-facing offline acceptance evidence now used by
Spindle. It summarizes sanitized results from the local fake harness and the
local Huichuan runtime mirror without importing private hardware source,
credentials, board addresses, or raw site logs.

## Safety Scope

The evidence is `offline_ok`.

It covers:

- local pytest;
- fixture refresh dry-run;
- profile schema drift checks;
- sanitized SOEM trace to profile regression;
- XML profile batch regression;
- real-report replay batch regression;
- generated JSON and Markdown acceptance summaries.

It does not approve or execute:

- SSH or scp;
- reboot or remoteproc reload;
- start-bus or stop-bus;
- `0x86`;
- `0x41F1`;
- takeover;
- IO output;
- robot motion.

## Standalone Fake Harness Acceptance

Project: `ethercat-fake-harness`

Source evidence:

- `tools/generated/latest_offline_regression_summary.json`
- `tools/generated/latest_offline_regression_summary.md`

Latest accepted result:

- offline acceptance: PASS, 6/6 steps;
- pytest: 24 passed;
- fixture refresh dry-run: PASS, 40 planned, 0 copied;
- profile schema drift: PASS, 5 documents, 10 profiles, 0 errors;
- SOEM trace batch: PASS, 3/3 cases, 0 failed;
- XML batch: PASS, 3/3 cases, 0 failed;
- replay batch: PASS, 15/15 cases, 0 failed.

## Huichuan Runtime Offline Mirror Acceptance

Project: `huichuan-robot-runtime/mix_protocol`

Source evidence:

- `tools/generated/latest_offline_regression_summary.json`
- `tools/generated/latest_offline_regression_summary.md`

Latest accepted result:

- static profile tests: PASS, 16/16;
- offline acceptance: PASS, 5/5 steps;
- pytest inside acceptance: 29 passed;
- fixture refresh dry-run inside acceptance: PASS, 40 noop, 0 copied;
- profile schema drift: PASS, 5 documents, 10 profiles, 0 errors;
- XML batch: PASS, 3/3 cases, 0 failed;
- replay batch: PASS, 15/15 cases, 0 failed.

The `40 noop / 0 copied` refresh result is expected inside the Huichuan runtime
mirror. It confirms the refresh planner resolves the existing local fixtures
without copying or overwriting them.

## Report Contract

The SOEM trace, XML, and replay batch reports keep the schema version 2
contract:

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

Each case includes:

- `inputs`
- `artifacts`
- `steps`

The one-command acceptance summary uses:

- `schema=fake_ecat_offline_acceptance`
- `schema_version=1`
- `offline_classification=offline_ok`
- `result`
- `totals`
- `steps`

## Agent Behavior Expected From This Evidence

Spindle should answer that the current offline acceptance state is green for
both the standalone fake harness and the Huichuan runtime mirror. For the
standalone fake harness, it should mention that sanitized SOEM traces now enter
the same offline evidence chain as XML and replay reports. It should also state
that this evidence does not authorize board, bus, output gate, IO, firmware, or
robot-motion actions.

For hardware-adjacent work, the next safe step is only to draft a read-only run
sheet. Executing that sheet requires explicit `board_required` approval.
