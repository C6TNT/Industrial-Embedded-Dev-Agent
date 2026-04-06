# Real Bench To Formal Merge Map

## Goal

This note maps the real-bench workflow outputs to the formal merge pipeline outputs.

Use it when you want to answer questions like:

- after I run the first real bench session, where do the files go?
- which bench artifact becomes a candidate draft?
- which candidate becomes a pending asset?
- which pending asset later becomes patch, preview, report, and checklist input?

---

## Big Picture

The full flow is:

1. real bench preparation
2. first read-only execution and evidence capture
3. session close-out
4. candidate export
5. candidate review
6. pending promotion
7. formal merge preparation
8. canonical review bundles
9. manual canonical merge

In short:

- real bench creates evidence
- evidence creates candidate drafts
- reviewed drafts enter `data/pending/`
- pending assets feed the formal merge chain

---

## Stage 1: Real Bench Preparation

Main entry:

```bash
ieda tools prep-real-bench --session-id <session-id> --label "<label>"
```

Main outputs:

- `reports/real_bench_prep/<session-id>/00_index.md`
- `reports/real_bench_prep/<session-id>/01_readiness_checklist.md`
- `reports/real_bench_prep/<session-id>/02_first_run_record.md`
- `reports/real_bench_prep/<session-id>/03_issue_capture.md`
- `reports/real_bench_prep/<session-id>/04_session_review.md`
- `reports/real_bench_prep/<session-id>/doctor_snapshot.json`
- `reports/real_bench_prep/<session-id>/plan_seed.json`

What this stage does:

- prepares the bench session container
- stores the initial runtime snapshot
- stores the first low-risk seed request

What it does not do:

- it does not create formal merge candidates yet

---

## Stage 2: First Read-Only Bench Capture

Main entry:

```bash
ieda tools kickoff-real-bench "reports/real_bench_prep/<session-id>/plan_seed.json" --render-all
```

Main outputs:

- `reports/real_bench_prep/<session-id>/kickoff_outputs/bench_pack.json`
- `reports/real_bench_prep/<session-id>/kickoff_outputs/first_run.md`
- `reports/real_bench_prep/<session-id>/kickoff_outputs/session_review.md`
- `reports/real_bench_prep/<session-id>/kickoff_outputs/run_summary.json`
- `reports/real_bench_prep/<session-id>/kickoff_outputs/run_summary.md`

What this stage does:

- captures the first structured evidence package
- renders human-readable first-run and session-review drafts
- preserves the first machine-readable summary

What this stage feeds next:

- `finish-real-bench`

---

## Stage 3: Bench Session Close-Out

Main entry:

```bash
ieda tools finish-real-bench --session-id <session-id>
```

Main outputs:

- `reports/real_bench_prep/<session-id>/finish_outputs/session_bundle.md`
- `reports/real_bench_prep/<session-id>/finish_outputs/final_summary.json`
- `reports/real_bench_prep/<session-id>/finish_outputs/final_summary.md`
- `reports/real_bench_prep/<session-id>/finish_outputs/candidate_exports/case_candidate.md`
- `reports/real_bench_prep/<session-id>/finish_outputs/candidate_exports/log_candidate.json`
- `reports/real_bench_prep/<session-id>/finish_outputs/candidate_exports/benchmark_candidate.json`

This is the first place where real bench outputs become potential dataset assets.

Mapping:

- bench evidence -> `case_candidate.md`
- bench evidence -> `log_candidate.json`
- bench evidence -> `benchmark_candidate.json`

These are still session-local candidate drafts, not shared dataset assets yet.

---

## Stage 4: Candidate Review

Main entry:

```bash
ieda tools review-finish-candidates --session-id <session-id>
```

Main outputs:

- `reports/real_bench_prep/<session-id>/finish_outputs/candidate_review/review_summary.json`
- `reports/real_bench_prep/<session-id>/finish_outputs/candidate_review/review_summary.md`

What this stage decides:

- whether the candidate wording is usable
- whether the tags look right
- whether the benchmark candidate is worth keeping

What this stage feeds next:

- `promote-finish-candidates`

---

## Stage 5: Pending Promotion

Main entry:

```bash
ieda tools promote-finish-candidates --session-id <session-id>
```

Main outputs:

- `data/pending/cases/...`
- `data/pending/logs/...`
- `data/pending/benchmarks/...`
- `data/pending/promotion_records/...`
- `data/pending/benchmarks/pending_benchmark_candidates.jsonl`

This is the key transition point:

- session-private candidate drafts
- become repository-level pending assets

At this point the outputs are no longer just bench notes.
They become formal merge inputs.

---

## Stage 6: Formal Merge Preparation

Main entries:

```bash
ieda tools plan-pending-merge
ieda tools prepare-formal-merge
ieda tools apply-formal-merge --dry-run
ieda tools apply-formal-merge --execute
```

Main outputs:

- `data/pending/merge_plan/...`
- `data/pending/formal_merge_assistant/...`
- `data/pending/formal_merge_assistant/staging/...`

Mapping from pending assets:

- `data/pending/cases/*`
  -> `materials_case_merge_candidates.md`
  -> staging materials candidates

- `data/pending/logs/*`
  -> material index patch suggestions
  -> merge assistant review context

- `data/pending/benchmarks/*`
  -> `benchmark_append_candidates.jsonl`
  -> `benchmark_append_patch.jsonl`
  -> staging benchmark append patch

This stage does not write canonical files.
It only prepares increasingly realistic review bundles.

---

## Stage 7: Canonical Review Bundles

Main entries:

```bash
ieda tools canonical-merge-preflight
ieda tools canonical-patch-helper
ieda tools canonical-merge-preview
ieda tools canonical-merge-report
ieda tools canonical-merge-checklist
```

Main outputs:

- `data/pending/formal_merge_assistant/canonical_merge_preflight.*`
- `data/pending/formal_merge_assistant/canonical_patch_bundle/...`
- `data/pending/formal_merge_assistant/canonical_merge_preview/...`
- `data/pending/formal_merge_assistant/canonical_merge_report/...`
- `data/pending/formal_merge_assistant/canonical_merge_checklist/...`

What each layer means:

- preflight
  checks whether the canonical review is structurally safe to begin

- patch bundle
  shows append-style patch files for canonical targets

- preview
  shows what the merged files might look like

- report
  summarizes preflight + patch + preview in one place

- checklist
  answers whether manual canonical merge review can start

---

## Stage 8: Manual Canonical Merge

Main guide:

- `docs/manual_canonical_merge_guide.md`

This is where humans finally decide:

- which materials sections are worth keeping
- which material index entries are correct
- which benchmark append items deserve to become canonical

Canonical targets:

- `data/materials/...`
- `data/materials/material_index_v1.md`
- `data/benchmark/benchmark_v1.jsonl`

This is the first stage that can actually change canonical data, and it stays manual on purpose.

---

## Minimal Trace Map

If you only want the shortest mapping view, use this:

- `prep-real-bench`
  -> creates session container and `plan_seed.json`

- `kickoff-real-bench`
  -> creates `bench_pack.json`

- `finish-real-bench`
  -> creates `case_candidate.md`, `log_candidate.json`, `benchmark_candidate.json`

- `review-finish-candidates`
  -> creates review summary

- `promote-finish-candidates`
  -> copies reviewed candidates into `data/pending/`

- `prepare-formal-merge` / `apply-formal-merge`
  -> turns pending assets into review bundles and staging patches

- `canonical-*`
  -> turns those review bundles into preflight, patch, preview, report, and checklist layers

- manual guide
  -> applies final human curation into canonical files

---

## Related Docs

- `docs/real_bench_first_day_plan.md`
- `docs/real_bench_quickstart.md`
- `docs/real_bench_readiness_checklist.md`
- `docs/real_bench_first_run_template.md`
- `docs/real_bench_issue_capture_template.md`
- `docs/real_bench_session_review_template.md`
- `docs/formal_merge_workflow.md`
- `docs/formal_merge_quickstart.md`
- `docs/manual_canonical_merge_guide.md`
