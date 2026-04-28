# Offline Acceptance Demo Q&A

This page is a public demo script for the Agent. It uses sanitized offline
acceptance facts only. It does not expose board addresses, raw private logs,
local operator notes, credentials, or hardware-only commands.

## Demo Goal

Show that the Agent can answer the current offline verification status and keep
the hardware boundary explicit.

Preferred sources:

- `CUR-012`: public offline acceptance evidence.
- `CUR-008`: current EtherCAT Agent development guide.
- `CUR-011`: offline regression baseline.

## Try These Questions

1. What is the current 2026-04-28 offline acceptance status?
2. Which checks passed in the standalone fake harness?
3. Which checks passed in the Huichuan runtime mirror?
4. Does the latest offline acceptance evidence authorize board, bus, output
   gate, IO, firmware, or robot-motion actions?
5. What should change before a read-only board batch is allowed?
6. Which files summarize the current public evidence?
7. What is the safest next step if the network cannot push to GitHub?

## Expected Answer Shape

For the current acceptance status, the Agent should say that the state is green
and cite these facts:

- Standalone `ethercat-fake-harness`: acceptance PASS `5/5`, pytest
  `22 passed`, fixture refresh dry-run `40 planned / 0 copied`, schema drift
  `5 documents / 10 profiles / 0 errors`, XML batch `3/3 PASS`, replay batch
  `15/15 PASS`.
- `huichuan-robot-runtime` mirror: static profile `16/16 PASS`, acceptance PASS
  `5/5`, pytest inside acceptance `29 passed`, fixture refresh dry-run
  `40 noop / 0 copied`, schema drift `5 documents / 10 profiles / 0 errors`,
  XML batch `3/3 PASS`, replay batch `15/15 PASS`.

For safety questions, the Agent should say that the evidence is `offline_ok`
only. It does not authorize board access, bus control, output gate unlock, IO
output, firmware lifecycle actions, takeover, or robot motion.

For next-step questions, the Agent should prefer:

- public demo/documentation updates in `spindle-agent`;
- fixture provenance and offline regression rules in `ethercat-fake-harness`;
- a read-only Huichuan board run sheet that remains unexecuted until the user
  explicitly approves a `board_required` window.

## Public Evidence Files

- `data/materials/offline_acceptance_evidence_2026_04_28.md`
- `data/materials/offline_regression_baseline_2026_04_28.md`
- `docs/current_ethercat_agent_development_guide.md`
- `data/benchmark/benchmark_v1.jsonl`
- `data/chunks/doc_chunks_v1.jsonl`

## Demo Boundary

The demo can run local Agent commands, benchmarks, tests, chunk rebuilds, and
secret scans. It must not SSH, scp, reboot, reload remoteproc, deploy firmware,
start or stop the EtherCAT bus, send `0x86`, unlock `0x41F1`, write IO, perform
takeover, or move the robot.
