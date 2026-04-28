# Legacy CANopen / RPMsg Lessons - 2026-04-28

This material distills reusable engineering lessons from the legacy i.MX8MP
RPMsg + M7 + CANopen dual-drive chain. It is a sanitized summary, not a source
code import.

## Architecture Pattern

The historical chain used an A53/Linux-side program to send motion intent
through RPMsg to M7 firmware. M7 then translated the intent into FlexCAN /
CANopen servo control.

Reusable concepts:

- A53 owns orchestration, operator-facing scripts, and high-level commands.
- M7 owns deterministic control loops and fieldbus-facing work.
- RPMsg is the boundary between Linux intent and real-time execution.
- Serial feedback is valuable, but success criteria must not rely on a single
  channel or a single boot event.

## CANopen Control Lessons

The stable minimal control shape was:

- `RPDO1` carries operation mode, controlword, and target speed.
- SDO reads back mode display, statusword, actual speed, and actual position.
- The enable sequence follows the standard controlword progression:
  `0x06 -> 0x07 -> 0x0F`.
- Feedback should be checked from the drive-visible state, not only from program
  return codes.

The legacy interface notes also describe a service loop where RPDO is sent
periodically and SDO feedback is sampled at a slower cadence. That design idea
maps cleanly to later EtherCAT work: separate command cadence, feedback cadence,
and human-readable validation evidence.

## Dual-Drive Debug Lessons

Single-drive success did not prove dual-drive correctness. Dual-drive work added
new failure surfaces:

- node/axis mapping can be correct for one axis and wrong for another;
- cold-start behavior can expose incomplete baseline alignment;
- verification scripts can create false failures if they contend for the same
  communication channel;
- object-page state may be more trustworthy than high-level script success.

The useful habit is to compare three sources before diagnosing a control issue:

- M7-side logs;
- A53-side verification output;
- drive/object-page state.

## Safety Boundary

The legacy chain included hardware deployment patterns such as WSL build,
SSH/SCP, reboot, Linux-side startup, serial capture, and board-specific
assumptions. Those are historical facts only. They are `board_required` or
higher when executed and must never be auto-executed by Spindle Agent.

For public Agent use, preserve the lessons and discard the private execution
details.

## Reuse In Current Projects

- In `huichuan-robot-runtime`, use these lessons to plan read-only comparison,
  validation order, and communication-channel ownership.
- In `ethercat-fake-harness`, convert verified failures into replay cases or
  profile boundary tests when possible.
- In `spindle-agent`, expose the lessons as searchable summaries and benchmark
  items, not raw code.
