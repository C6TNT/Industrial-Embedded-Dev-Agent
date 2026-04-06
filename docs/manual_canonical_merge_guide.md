# Manual Canonical Merge Guide

## Purpose

This guide describes the final human-driven merge step after the formal merge pipeline has already produced:

- pending candidates
- formal merge assistant outputs
- staging outputs
- canonical preflight
- canonical patch bundle
- canonical preview bundle
- canonical report
- canonical checklist

It is intentionally conservative:

- the toolchain prepares almost everything
- but the last canonical write remains manual

---

## When To Use This Guide

Use this guide only after all of the following are true:

1. `ieda tools canonical-merge-checklist` says `ready_for_manual_canonical_merge_review = true`
2. `ieda tools canonical-merge-preflight` has no unresolved blockers
3. You have manually reviewed the patch bundle and preview bundle
4. You are ready to curate final wording before touching canonical files

If any of those are still false, stop here and return to the earlier pipeline.

---

## Recommended Review Order

Open files in this order:

1. `data/pending/formal_merge_assistant/canonical_merge_checklist/canonical_merge_checklist.md`
2. `data/pending/formal_merge_assistant/canonical_merge_report/canonical_merge_report.md`
3. `data/pending/formal_merge_assistant/canonical_merge_preflight.md`
4. `data/pending/formal_merge_assistant/canonical_patch_bundle/canonical_patch_manifest.md`
5. `data/pending/formal_merge_assistant/canonical_merge_preview/canonical_merge_preview_manifest.md`
6. `data/pending/formal_merge_assistant/canonical_patch_bundle/data/materials/materials_case_merge_candidates.md`
7. `data/pending/formal_merge_assistant/canonical_patch_bundle/data/materials/material_index_v1.append.md`
8. `data/pending/formal_merge_assistant/canonical_patch_bundle/data/benchmark/benchmark_v1.append.jsonl`
9. `data/pending/formal_merge_assistant/canonical_patch_bundle/recommended_commit_split.md`

This order reduces risk:

- checklist tells you whether you should proceed at all
- report gives the summary
- preflight shows structural blockers
- patch and preview show what would change
- final patch files are reviewed only after the higher-level checks are understood

---

## Manual Merge Steps

### 1. Confirm Readiness

Before editing canonical files, confirm:

- no duplicate benchmark IDs remain
- no material index conflicts remain
- staging files exist and look reasonable
- preview output still matches your intended final shape

If any of those fail, do not edit canonical files yet.

### 2. Curate Materials Content

Review:

- `materials_case_merge_candidates.md`

Decide whether each section should:

- become a new materials file
- be merged into an existing materials file
- stay only in pending for later
- be discarded

Do not blindly copy the whole candidate file if parts are noisy or overly session-specific.

### 3. Curate Material Index Entries

Review:

- `material_index_v1.append.md`

For each proposed line:

- confirm the identifier style is consistent
- confirm the category is correct
- confirm the target path is the final canonical path, not a pending path

If a materials file name changes during curation, update the index entry accordingly.

### 4. Curate Benchmark Appends

Review:

- `benchmark_v1.append.jsonl`

For each candidate item:

- confirm the `id` is stable and unique
- confirm the wording is useful beyond one bench session
- confirm the expected answer or guardrail still matches canonical policy
- remove session-local noise

Only append benchmark items that are worth keeping as long-term regression assets.

### 5. Apply Canonical File Edits Manually

Only after curation:

1. update canonical materials files under `data/materials/`
2. update `data/materials/material_index_v1.md`
3. append reviewed items into `data/benchmark/benchmark_v1.jsonl`

Recommended rule:

- materials changes first
- index changes second
- benchmark changes last

This keeps the review story cleaner.

### 6. Re-Run Local Validation

After manual edits, run:

```bash
ieda check
```

If your canonical changes also affect evaluation wording or retrieval expectations, optionally run:

```bash
ieda check --include-offline --include-rag --rag-type tool_safety
```

Do not commit canonical changes before checks pass.

---

## Suggested Commit Split

Prefer at least two commits:

### Commit 1: materials and index

Typical scope:

- `data/materials/...`
- `data/materials/material_index_v1.md`

Reason:

- document curation is easier to review separately

### Commit 2: benchmark appends

Typical scope:

- `data/benchmark/benchmark_v1.jsonl`

Reason:

- benchmark changes affect regression baselines
- they are easier to reason about when isolated

If the materials change is especially large, split it again into:

- new materials files
- material index updates

---

## Stop Conditions

Stop and do not proceed with canonical edits if:

- preflight still reports failed checks
- preview output and patch output disagree in a meaningful way
- the candidate wording is still too session-specific
- benchmark IDs collide with existing canonical items
- you are unsure whether a candidate belongs in materials or only in pending

In those cases, return to:

- `review-finish-candidates`
- `promote-finish-candidates`
- `plan-pending-merge`
- `prepare-formal-merge`
- `apply-formal-merge --dry-run`

---

## Minimal Safe Path

If you want the shortest safe path, use this order:

1. `ieda tools canonical-merge-checklist`
2. `ieda tools canonical-merge-report`
3. inspect `canonical_patch_bundle`
4. inspect `canonical_merge_preview`
5. manually curate canonical files
6. `ieda check`
7. commit curated changes in small reviewable commits

That path keeps the final write manual while still fully using the preparation pipeline.
