# Spindle GSD State

## Current State

- Project name selected: **Spindle**.
- Local folder renamed to `D:\桌面\Spindle`.
- Primary engineering material workspace: `D:\桌面\test`.
- Existing Python package path remains `industrial_embedded_dev_agent` for compatibility.
- New CLI alias planned through package metadata: `spindle`.
- GitHub CLI is not installed locally, so GitHub repository description cannot be edited from this machine yet.

## Product Direction

Spindle should first become a wrapper around existing coding CLI / Agent SDK capabilities.

The early focus is:

1. tools;
2. memory;
3. raw search;
4. safe command planning;
5. offline verification.

Avoid spending early effort on:

- custom harness loop;
- large knowledge base ingestion;
- direct hardware execution;
- deleting or rewriting stable engineering scripts.

## Active Phase

Phase 1-3 offline MVP foundation.

## Completed After Spindle Rename

- Added `material-inventory` for search-first workspace mapping.
- Added `material-status` for a compact material workspace health and boundary dashboard.
- Added `material-search` for raw source/document/log/XML/profile search.
- Added `material-tools` for existing script risk classification.
- Added `material-tool-plan` for single-tool execution planning.
- Added `material-run-tool` for gated execution of `offline_ok` scripts only.
- Added `material-draft-fact` for source-checked curated memory drafts.
- Verified the material workspace at `D:\桌面\test`.
- Kept hardware-facing scripts blocked from autonomous execution.

## Current Offline Automation Step

Continue offline automation toward a usable wrapper:

1. keep search-first material access as the primary knowledge path;
2. keep report-oriented summaries for inventory/tool risk visible through `material-status`;
3. run pytest, benchmark, secret scan, and pre-push gates before handoff;
4. keep all hardware actions behind audit/checklist only.

## Completed Board-Only Read Diagnostics

- Added `board-status` for SSH reachability, `/home`, `a53_send_ec_profile`, and `rpmsg_auto_open.py` presence checks.
- Added `rpmsg-health` for `/dev/rpmsg*`, `rpmsg_auto_open.py --print-dev`, and RPMsg-related log collection.
- Added `m7-health` for M7/RPMsg/EtherCAT log markers without remoteproc lifecycle operations.
- Added `ethercat-query-readonly` for `a53_send_ec_profile --query` only; no start/stop/control path is present.
- Added `board-report` to write JSON/Markdown summaries under `reports/board_only/`.
- All board-only commands default to dry-run. Real SSH requires `--execute` and a board-only read window.
- Safety guard blocks start/stop bus, `0x86`, `0x41F1`, robot motion, IO output, firmware flash/hot reload, and remoteproc lifecycle actions from this path.
- Unit tests cover dry-run behavior, safety blocking, SSH failure attribution, RPMsg missing device, M7 boot failure markers, query parsing, and report generation.

## External GSD SDK Note

The external `gsd-sdk auto --project-dir .` command exists, but the underlying model session was not logged in during the previous attempt. Spindle therefore keeps local `.planning` artifacts as the source of truth until the SDK login state is fixed.
## Last Offline GSD Run

- created_at: 2026-04-28T09:53:58.087159+08:00
- passed: True
- next_action: ready_for_review_or_push
- guarded_local_gate: PASS
- pre_push_static_gate: PASS
- hardware_scope: board diagnostics dry-run only; --execute requires a board-only read window; blocked for start-bus/stop-bus/0x86/0x41F1/robot motion/IO/remoteproc lifecycle/firmware
