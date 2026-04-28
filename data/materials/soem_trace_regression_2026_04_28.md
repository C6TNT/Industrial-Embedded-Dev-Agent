# SOEM Trace Regression - 2026-04-28

This material records the new offline bridge from sanitized SOEM evidence to
dynamic EtherCAT profile regression. It is public-safe because it summarizes the
workflow and sanitized sample shape without importing raw packet captures,
private board addresses, credentials, live bus logs, or hardware commands.

## Safety Scope

Classification: `offline_ok`.

The SOEM trace regression parses text fixtures only. It does not:

- open a network interface;
- sniff a live EtherCAT bus;
- SSH or scp;
- reboot or reload remoteproc;
- start or stop EtherCAT;
- write PDO outputs;
- unlock `0x41F1`;
- send IO output, takeover, or robot motion commands.

Live packet capture or live SOEM logging is at least `board_required`. If the
live system also performs IO output or robot motion, the boundary rises to
`io_required` or `robot_motion_required`.

## Offline Flow

The fake harness project now supports this chain:

```text
sanitized SOEM trace
  -> profile candidate JSON
  -> validate_ec_profile.py
  -> fake_ecat_harness.py replay
  -> batch JSON/Markdown report
```

The parser extracts:

- slave identity: vendor ID, product code, revision, name, slave, logical axis;
- strategy and mode when present;
- RPDO/RxPDO and TPDO/TxPDO mapping entries;
- non-PDO SDO init writes such as `0x6060`;
- process-data size hints such as `ob`, `ib`, and `wkc`;
- state hints such as OP.

PDO mapping and assignment SDO writes, for example `0x1600..0x17FF`,
`0x1A00..0x1BFF`, `0x1C12`, and `0x1C13`, are counted as configuration
evidence instead of being copied into `extra_sdo`. This avoids duplicating PDO
mapping in the runtime profile candidate.

## Current Regression Result

Project: `ethercat-fake-harness`

New tools:

- `tools/fake_ecat_harness/soem_trace_parser.py`
- `tools/fake_ecat_harness/run_soem_trace_batch.py`
- `tools/fake_ecat_harness/soem_trace_cases/*.log`

Latest local result:

- pytest: `24 passed`;
- offline acceptance: `PASS`, `6/6`;
- SOEM trace batch: `PASS`, `3/3`;
- XML batch: `PASS`, `3/3`;
- replay batch: `PASS`, `15/15`.

The SOEM trace batch is now included in
`tools/fake_ecat_harness/run_offline_acceptance.py` between schema drift and XML
batch regression.

## Agent Behavior Expected From This Evidence

Spindle should answer that SOEM is already the EtherCAT master layer in the
private Huichuan runtime, while the public-safe offline work should focus on
SOEM-adjacent evidence:

- convert sanitized SOEM logs into profile candidates;
- validate profile transport and PDO boundaries offline;
- replay the candidate in fake harness;
- store the resulting report as regression evidence;
- keep hardware actions behind explicit boundary approval.

Spindle must not claim that an offline SOEM trace PASS authorizes a board run,
bus control, output gate unlock, IO output, or robot motion. It only means the
profile candidate is internally consistent enough for the next review step.
