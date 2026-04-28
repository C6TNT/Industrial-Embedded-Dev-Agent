# Spindle Requirements

## Functional Requirements

1. Spindle must treat `D:\桌面\test` as the primary Xi-factory engineering material workspace.
2. Spindle must start with raw source search, file inventory, and curated memory instead of a heavy knowledge base.
3. Spindle must expose a safe wrapper over existing offline scripts before building any custom harness loop.
4. Spindle must keep the current Python package import path stable until a deliberate migration is planned.
5. Spindle must support:
   - project search;
   - engineering Q&A with source evidence;
   - planning;
   - safe tool auditing;
   - offline checks;
   - report import and summary;
   - curated fact drafting;
   - board-only diagnostic dry-run planning and explicit `--execute` read checks.
6. Spindle must stop before autonomous coding starts until the user confirms.

## Safety Requirements

1. No real hardware command may run automatically.
2. Board-only diagnostic commands must default to dry-run and must require `--execute` before SSH.
3. A `--execute` board diagnostic window may only run read-only checks for SSH, helper files, RPMsg devices/logs, M7 logs, and `a53_send_ec_profile --query`.
4. Any request involving robot, IO, EtherCAT bus control, control words, output gates, firmware, or M7 lifecycle must stop at plan/audit/checklist.
5. The safe default is `offline_ok`.
6. Existing harness and fake tools may be reused, but early work must not focus on rewriting harness loops.

## Search And Memory Requirements

1. Raw search must cover code, docs, XML, JSON, logs, reports, and scripts.
2. Curated memory must only contain stable facts with sources.
3. Noisy files, generated reports, large manuals, and temporary probes should stay searchable but not automatically promoted to canonical memory.
4. RAG/chunks may remain as support, but they should not become the only source of truth.

## Verification Requirements

Before considering an offline GSD iteration complete, run:

```powershell
python -m pytest tests -q
python -m industrial_embedded_dev_agent benchmark run --engine rules
python -m industrial_embedded_dev_agent benchmark run --engine tools --type tool_safety
python -m industrial_embedded_dev_agent benchmark run --engine rag --type tool_safety
python -m industrial_embedded_dev_agent tools secret-scan
python -m industrial_embedded_dev_agent tools pre-push-check --skip-local-checks
```

For larger changes, also run:

```powershell
python -m industrial_embedded_dev_agent check --include-offline --include-rag --rag-type tool_safety --no-write-summary
```
