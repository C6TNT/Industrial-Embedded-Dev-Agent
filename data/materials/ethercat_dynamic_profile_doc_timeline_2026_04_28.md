# EtherCAT Dynamic Profile Documentation Timeline - 2026-04-28

This material summarizes the private documentation timeline that led from early
CANopen/RPMsg work to the current EtherCAT Dynamic Profile, robot position mode,
and fake harness baseline.

## Timeline

2026-03:

- i.MX8MP platform bring-up, M7 deployment learning, serial validation, RPMsg
  basics, EtherCAT/CANopen concepts, PDO/SDO concepts, and minimal control-chain
  experiments.

2026-04-09:

- CANopen dual-drive documentation and replay-style lessons were consolidated.
  The key lesson was that single-drive validation is only a baseline; dual-drive
  work needs independent axis/node handling, feedback comparison, and careful
  validation ordering.

2026-04-20:

- Single-drive EtherCAT usage documentation appeared as the bridge from the
  CANopen learning phase into EtherCAT servo control.

2026-04-21 to 2026-04-23:

- Single-drive EtherCAT expanded into dual-drive, multi-drive, dynamic
  allocation, and dynamic reconfiguration documentation.
- These documents are the private history behind the current Dynamic Profile
  focus: profile generation, PDO shape, driver adaptation, and repeatable
  validation.

2026-04-24:

- Robot single-axis EtherCAT dynamic-link material and robot position-mode
  material were produced. The focus shifted from drive-only validation to robot
  axis mapping and controlled position-mode evidence.

2026-04-27:

- Robot position-mode documentation was updated.
- Fake Harness documentation was added, turning accumulated real-project
  knowledge into an offline validation and replay strategy.

2026-04-28:

- The current three-project workspace froze a stable offline baseline:
  `huichuan-robot-runtime` for real hardware work, `ethercat-fake-harness` for
  local replay/profile validation, and `spindle-agent` for public searchable
  knowledge and benchmarked answers.

## Knowledge Base Use

Spindle should use this timeline to answer project-history questions and to
explain why the current baseline emphasizes:

- static profile validation;
- PDO layout checks;
- replayable fake harness reports;
- robot position-mode evidence;
- safety boundaries before board, IO, firmware, or motion work.

The timeline is public-safe because it records high-level project evolution
rather than private documents, board credentials, local paths, raw logs, or
hardware execution commands.
