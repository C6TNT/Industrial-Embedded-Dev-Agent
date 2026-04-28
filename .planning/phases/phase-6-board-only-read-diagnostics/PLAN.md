# Phase 6 Plan: Board-Only Read Diagnostics

## Goal

Support development-board-only work when robot and IO hardware are unavailable, while keeping the path read-only and auditable.

## Tasks

1. Add dry-run-first commands for board reachability, RPMsg, M7 log hints, EtherCAT query, and combined report generation.
2. Require `--execute` before any SSH command is run.
3. Keep the remote scripts read-only and block bus lifecycle, control words, output gates, robot motion, IO output, firmware flash, remoteproc lifecycle, and M7 hot reload.
4. Attribute common failures: SSH unreachable, missing helper files, no RPMsg device, RPMsg open failure, M7 boot failure markers, query missing, query failed, query unparsed, timeout.
5. Write JSON/Markdown reports under `reports/board_only/`.
6. Cover the path with pytest and document the boundary in README, roadmap, and GSD planning files.

## Completion Criteria

- `spindle tools board-status`, `rpmsg-health`, `m7-health`, `ethercat-query-readonly`, and `board-report` dry-run without touching hardware.
- `board-report --execute` remains explicitly opt-in and still excludes start/stop/control/motion/IO/firmware actions.
- Unit tests cover dry-run, report generation, safety blocking, and failure attribution.
- Full local gates pass.

## Safety

This phase is board-required only when `--execute` is present. It never starts/stops EtherCAT, never writes `0x86`, never unlocks `0x41F1`, never moves a robot, never writes IO, and never changes M7/remoteproc lifecycle state.
