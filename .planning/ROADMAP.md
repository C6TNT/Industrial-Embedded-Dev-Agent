# Spindle GSD Roadmap

## Phase 0: Identity And Boundary

Goal: rename the project to Spindle and keep the hardware boundary explicit.

Acceptance:

- Repository folder, README, package metadata, and planning docs use Spindle.
- The Python package import path can stay stable for now.
- GSD safety boundary blocks board, robot, IO, firmware, and bus actions.

## Phase 1: Search-First Engineering Index

Goal: make raw project search useful before building a heavy knowledge base.

Acceptance:

- Inventory `D:\桌面\test` and classify source, docs, tools, logs, XML, JSON profiles, reports, and manuals.
- Provide CLI commands for targeted source search and file lookup.
- Prefer raw text search plus file reading over generic RAG for unfamiliar questions.

## Phase 2: Curated Memory

Goal: store only stable engineering facts that have been reviewed or proven by reports/tests.

Acceptance:

- Material facts are separated into `CUR-*`, `LOG-*`, `CASE-*`, and tool entries.
- Memory records include source file, confidence, date, and whether hardware validation is required.
- No bulk dumping of noisy folders into canonical memory.

## Phase 3: Safe Tool Wrapper

Goal: expose existing engineering scripts through guarded tool metadata.

Acceptance:

- Offline scripts can run automatically.
- Hardware-facing scripts are listed but stop at audit/checklist.
- Tool metadata includes `offline_ok`, `board_required`, `robot_motion_required`, `io_required`, or `firmware_required`.

## Phase 4: Coding CLI / Agent SDK Wrapper Prototype

Goal: use an existing coding CLI or Agent SDK as the execution substrate before writing a custom runtime.

Acceptance:

- Wrapper can answer, search, plan, edit, run offline checks, and summarize reports.
- Wrapper preserves safety gates before executing any command.
- No custom harness loop is built in this phase.

## Phase 5: Offline Automation Readiness

Goal: run autonomous offline development safely after user approval.

Acceptance:

- GSD plan is complete.
- Search, memory, and tool boundaries are defined.
- Verification command list is ready.
- Stop before any real hardware/device action; offline implementation can continue under the approved boundary.

## Phase 6: Board-Only Read Diagnostics

Goal: when only the development board is available, collect read-only health evidence without touching robot motion, IO, bus lifecycle, or firmware lifecycle.

Acceptance:

- `board-status`, `rpmsg-health`, `m7-health`, `ethercat-query-readonly`, and `board-report` default to dry-run.
- `--execute` is required before any SSH command is run.
- The generated report captures pass/fail, timeout, SSH failure, missing helper files, RPMsg absence, M7 boot markers, and query parse status.
- The read-only path blocks `start-bus`, `stop-bus`, `0x86`, `0x41F1`, IO output, robot motion, remoteproc lifecycle, firmware flashing, and M7 hot reload.

## Phase 7: Later Runtime Replacement

Goal: only after the wrapper hits limitations, evaluate replacing parts with a custom runtime inspired by projects such as pi-mono.

Acceptance:

- Wrapper limitations are documented.
- Replacement scope is narrow and justified.
- Existing tools and memory are preserved.
