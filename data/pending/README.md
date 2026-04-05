# Pending Dataset Area

`data/pending/` is the review-and-promotion buffer between bench-generated candidate drafts and the formal project dataset.

It exists for one reason:
- bench automation can generate useful candidate artifacts quickly
- but those artifacts should not be merged into the formal dataset without human review

## What Goes Here

- `cases/`
  Candidate case summaries promoted from `finish-real-bench`
- `logs/`
  Candidate log-style structured evidence exported from bench sessions
- `benchmarks/`
  Candidate benchmark items and the aggregated `pending_benchmark_candidates.jsonl`
- `promotion_records/`
  Machine-readable records showing what was promoted, from which session, and where it landed

## What Does Not Belong Here

- raw runtime dumps that should stay under `reports/`
- temporary scratch files with no review value
- direct edits to the formal benchmark or material index without review

## Recommended Flow

1. Run `ieda tools finish-real-bench --session-id <id>`
2. Run `ieda tools review-finish-candidates --session-id <id>`
3. Manually inspect the generated review summary and candidate files
4. Run `ieda tools promote-finish-candidates --session-id <id>`
5. Periodically merge reviewed pending items into the formal dataset in a separate, explicit change

## Merge Rule

Promotion into `data/pending/` means:
- the candidate is considered useful enough to keep
- but it is still not a formal benchmark, formal case, or formal material entry

Formal dataset updates should happen in a separate reviewable step, so the repo always keeps a clear boundary between:
- generated candidate content
- curated canonical content
