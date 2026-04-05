# Stub Scenario Cases V1

## Purpose

This note captures the no-hardware WSL stub scenarios that are now part of the project workflow.
They are not a replacement for real bench validation, but they are useful for:

- parser regression
- compare-pack regression
- session bundle rendering
- issue / review template rehearsal

## Scenario Catalog

### SCENARIO-001 nominal

- Intent: provide a readable transport with stable idle snapshots on both axes
- Typical signal:
  - `OpenRpmsg 0`
  - `statusCode=33`
  - `axisStatus=4097`
  - encoder increments slowly across polls
- Recommended use:
  - baseline bench-pack capture
  - session bundle smoke test
  - compare-pack left-side baseline

### SCENARIO-002 encoder_stall

- Intent: simulate a readable transport where encoder feedback stops changing
- Typical signal:
  - `OpenRpmsg 0`
  - transport still marked readable
  - encoder values remain flat across polls
- Recommended use:
  - compare-pack regression
  - stagnant feedback rehearsal
  - case drafting for “link alive but motion feedback stale”

### SCENARIO-003 axis1_fault

- Intent: simulate a readable transport where axis1 returns an abnormal servo snapshot
- Typical signal:
  - `OpenRpmsg 0`
  - axis0 remains nominal
  - axis1 shows `errorCode=16`, `statusCode=144`, `axisStatus=9473`
- Recommended use:
  - issue capture template rehearsal
  - compare-pack regression for single-axis anomalies
  - session review rehearsal for “stable axis vs abnormal axis”

### SCENARIO-004 open_rpmsg_fail

- Intent: simulate degraded transport initialization before a meaningful snapshot is captured
- Typical signal:
  - `OpenRpmsg -1`
  - subsequent reads degrade or return invalid values
- Recommended use:
  - RPMsg handshake troubleshooting rehearsal
  - degraded transport summary checks
  - pre-bench doctor / mode / recovery workflow drills

## Suggested Mapping To Project Assets

| Scenario | Suggested issue_category | Suggested case focus | Suggested next action |
|---|---|---|---|
| `nominal` | `verification_tooling` | readable baseline | archive as baseline snapshot |
| `encoder_stall` | `verification_tooling` | feedback stagnation | compare with previous pack and inspect encoder trend |
| `axis1_fault` | `verification_tooling` | single-axis abnormal snapshot | preserve evidence and cross-check with vendor tool on the real bench |
| `open_rpmsg_fail` | `rpc_rpmsg` | transport handshake degradation | run `probe_rpmsg_handshake` style checks first |

## Benchmark Seeds

These scenarios are suitable seeds for:

- knowledge QA about offline rehearsal boundaries
- log attribution around `OpenRpmsg` degradation
- compare-pack difference summaries
