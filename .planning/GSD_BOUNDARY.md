# Spindle GSD Automation Boundary

Spindle is allowed to automate only offline engineering work. Its default posture is search-first, memory-curated, and tool-guarded.

## Allowed Autonomous Scope

GSD may automatically execute:

- code edits inside this repository;
- unit tests and benchmark commands;
- raw project search and inventory generation;
- curated memory draft generation;
- RAG and chunks updates when they are supporting evidence, not the only source of truth;
- fake harness/replay/report summary generation using existing tools;
- board-only diagnostic dry-runs, including `board-status`, `rpmsg-health`, `m7-health`, `ethercat-query-readonly`, and `board-report` without `--execute`;
- secret scanning;
- pre-push static checks;
- README/docs/planning updates.

## Blocked Autonomous Scope

GSD must not automatically execute:

- `ssh`;
- `scp`;
- `reboot`;
- real `start-bus`;
- real `stop-bus`;
- `0x86` control word;
- `0x41F1` unlock;
- robot motion;
- IO output;
- welding output;
- limit output;
- `remoteproc` lifecycle operations;
- firmware flashing;
- M7 hot reload;
- direct execution of hardware-facing scripts from `D:\桌面\test\mix_protocol\tools`.

## Board-Only Read Diagnostics

GSD may plan board diagnostics by running the commands without `--execute`. In that mode the commands only render the SSH command and remote read-only script; they do not contact the board.

Allowed dry-run examples:

```powershell
spindle tools board-status
spindle tools rpmsg-health
spindle tools m7-health
spindle tools ethercat-query-readonly
spindle tools board-report
```

Commands with `--execute` require a board-only read window and must remain read-only. They may inspect SSH reachability, `/dev/rpmsg*`, M7 logs, helper-file presence, and `a53_send_ec_profile --query`.

The board-only path must not include `start-bus`, `stop-bus`, `0x86`, `0x41F1`, robot motion, IO output, welding/limit output, firmware flashing, remoteproc lifecycle operations, or M7 hot reload.

## Required Behavior For Blocked Scope

When a task crosses the boundary, GSD must stop at:

- a plan;
- an audit;
- a checklist;
- a fake/mock test;
- a required site-condition list;
- a manual confirmation request for a future hardware window.

## Knowledge Boundary

Do not try to turn all loose engineering material into a large knowledge base in the early phases. Prefer:

1. raw `rg`/file search;
2. direct file reading;
3. small source-backed inventory;
4. curated memory for stable facts;
5. RAG/chunks only as a supporting layer.

## Harness Boundary

Reuse the existing fake harness where useful. Do not spend early GSD effort rewriting the harness loop or building a custom simulation runtime.

## Canonical Audit Command

```powershell
python -m industrial_embedded_dev_agent tools audit-hardware-action "<request>"
```

## Canonical Offline Gate

```powershell
python -m industrial_embedded_dev_agent check --include-offline --include-rag --rag-type tool_safety --no-write-summary
```
