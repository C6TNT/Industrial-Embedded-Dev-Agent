# Spindle

## Purpose

Spindle is a search-first industrial engineering agent wrapper for the current EtherCAT Dynamic Profile project and the broader Xi-factory engineering material workspace under `D:\桌面\test`.

The name comes from the mechanical spindle: a central rotating element that connects motion, servo drives, robots, tools, and real engineering work.

The project helps maintain and improve offline tooling around:

- i.MX8MP A53 + RPMsg + M7 architecture knowledge.
- EtherCAT Dynamic Profile profile/topology reasoning.
- robot6 position-mode project facts.
- source search, curated memory, safe tool wrappers, fake harness reuse, replay, XML batch, report import, and benchmark regression.
- guarded tool planning with explicit hardware safety boundaries.

## Current Baseline

- Product name: `Spindle`.
- CLI entrypoints: `spindle` after install, or `python -m industrial_embedded_dev_agent` while keeping the existing Python package stable.
- Core commands: `benchmark`, `ask`, `search`, `chunks`, `check`, `tools`.
- Engineering commands: `secret-scan`, `pre-push-check`, `project-status`, `gsd-status`, `draft-fact`, `import-report`, `summarize-fake-regression`, `audit-hardware-action`.
- Board-only read diagnostics: `board-status`, `rpmsg-health`, `m7-health`, `ethercat-query-readonly`, and `board-report`; these default to dry-run and require `--execute` before SSH.
- Current benchmark shape: knowledge QA, log attribution, tool safety.
- Current verification gate: pytest, rules benchmark, tool safety benchmark, RAG tool-safety benchmark, offline stub samples, secret scan.

## Product Direction

Spindle starts as a wrapper around existing coding CLI / Agent SDK capabilities. Do not spend early effort building a new harness loop or a heavy knowledge base.

Priority order:

1. Search tools over raw source, docs, logs, XML, JSON, and reports.
2. Curated memory for stable, reviewed facts only.
3. Safe tool wrappers for existing offline scripts.
4. A coding CLI wrapper that can plan, inspect, edit, and verify.
5. Later runtime replacement only after the wrapper hits real limitations.

## GSD Role

GSD is enabled as the workflow layer for this repository. Its job is to continuously convert project goals into phases, plans, offline execution, verification, and updated project memory.

GSD is not allowed to control real industrial hardware. It may generate hardware test plans, checklists, audits, and mock/fake tests only.

## Non-Negotiable Boundary

Autonomous GSD work is limited to `offline_ok`.

Allowed without a hardware window:

- Board-only diagnostic dry-run planning with no `--execute`.

Blocked without explicit human-controlled hardware window:

- SSH/SCP/reboot to real boards.
- Real `start-bus` or `stop-bus`.
- `0x86` control word.
- `0x41F1` output gate unlock.
- Robot motion.
- IO, welding, or limit output.
- `remoteproc`, firmware flashing, M7 hot reload.

## Working Style

- Preserve existing working behavior.
- Do not delete legacy or canonical material without review.
- Prefer small, verifiable increments.
- Run checks after every offline iteration.
- Update `.planning/STATE.md` when a milestone meaningfully advances.
