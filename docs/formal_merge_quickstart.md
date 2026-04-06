# Formal Merge Quickstart

## Goal

This is the shortest practical guide for moving from reviewed bench candidates to a safe manual canonical merge.

Use it when you do not want to read the full workflow first.

---

## Fast Path

Run the formal merge pipeline in this order:

```bash
ieda tools review-finish-candidates --session-id <session-id>
ieda tools promote-finish-candidates --session-id <session-id>
ieda tools plan-pending-merge
ieda tools prepare-formal-merge
ieda tools apply-formal-merge --dry-run
ieda tools apply-formal-merge --execute
ieda tools canonical-merge-preflight
ieda tools canonical-patch-helper
ieda tools canonical-merge-preview
ieda tools canonical-merge-report
ieda tools canonical-merge-checklist
```

If checklist says the session is ready for manual review, continue with:

- `docs/manual_canonical_merge_guide.md`

---

## What Each Step Gives You

1. `review-finish-candidates`
   Reviews the raw bench-derived candidates.

2. `promote-finish-candidates`
   Moves approved drafts into `data/pending/`.

3. `plan-pending-merge`
   Shows what kinds of pending assets exist and where they probably belong.

4. `prepare-formal-merge`
   Builds the first formal merge assistant bundle.

5. `apply-formal-merge --dry-run`
   Shows what would change without staging anything.

6. `apply-formal-merge --execute`
   Writes a staging-only bundle, but still does not touch canonical files.

7. `canonical-merge-preflight`
   Checks duplicate IDs, missing targets, material index conflicts, and staging readiness.

8. `canonical-patch-helper`
   Produces append-style patch files for benchmark and material index updates.

9. `canonical-merge-preview`
   Produces preview files that show what merge results could look like.

10. `canonical-merge-report`
    Collects preflight, patch, and preview into a single overview.

11. `canonical-merge-checklist`
    Answers whether manual canonical merge review can begin.

---

## Read These Files In Order

When the pipeline is done, open files in this order:

1. `data/pending/formal_merge_assistant/canonical_merge_checklist/canonical_merge_checklist.md`
2. `data/pending/formal_merge_assistant/canonical_merge_report/canonical_merge_report.md`
3. `data/pending/formal_merge_assistant/canonical_merge_preflight.md`
4. `data/pending/formal_merge_assistant/canonical_patch_bundle/canonical_patch_manifest.md`
5. `data/pending/formal_merge_assistant/canonical_merge_preview/canonical_merge_preview_manifest.md`
6. `docs/manual_canonical_merge_guide.md`

If anything still looks wrong at that point, stop and go back to the earlier steps.

---

## Minimum Safety Rules

Do not manually merge into canonical files unless:

- checklist says readiness is true
- preflight has no unresolved blockers
- patch bundle and preview bundle both look reasonable
- candidate wording is no longer session-local noise

Even at the final step:

- keep materials changes separate from benchmark changes
- re-run `ieda check` after manual edits
- prefer small reviewable commits

---

## After Manual Canonical Edits

Run:

```bash
ieda check
```

If the canonical change affects retrieval or answer wording, also consider:

```bash
ieda check --include-offline --include-rag --rag-type tool_safety
```

Then commit in small pieces:

1. materials and material index
2. benchmark appends

---

## Related Docs

- `docs/formal_merge_workflow.md`
- `docs/manual_canonical_merge_guide.md`
- `data/pending/README.md`
