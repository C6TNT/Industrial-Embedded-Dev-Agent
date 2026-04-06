# Candidate Quality Check Plan

## Goal

This note defines the next automation target for the bench-to-dataset pipeline:

- `ieda tools candidate-quality-check --session-id <session-id>`

The purpose is to add one machine-first screening layer between:

- `finish-real-bench`

and:

- `review-finish-candidates`

So the human reviewer starts from cleaner candidate drafts.

---

## Why This Matters

Right now the flow already produces:

- `case_candidate.md`
- `log_candidate.json`
- `benchmark_candidate.json`

That is good, but it still means a human may review drafts that are:

- too short
- missing key fields
- too session-local
- weakly tagged
- poorly shaped for long-term benchmark use

A lightweight quality gate would reduce that noise before manual review starts.

---

## Proposed Entry Point

Planned command:

```bash
ieda tools candidate-quality-check --session-id <session-id>
```

Planned output directory:

- `reports/real_bench_prep/<session-id>/finish_outputs/candidate_quality_check/`

Planned outputs:

- `candidate_quality_check.json`
- `candidate_quality_check.md`

---

## Candidate Types To Check

### 1. Case Candidate

Target file:

- `finish_outputs/candidate_exports/case_candidate.md`

Checks to consider:

- file exists
- content length is above a minimal threshold
- includes session context
- includes at least one actionable finding or conclusion
- is not just a raw dump

### 2. Log Candidate

Target file:

- `finish_outputs/candidate_exports/log_candidate.json`

Checks to consider:

- file exists
- JSON parses
- `session_id` exists
- `tool_id` exists
- `risk_level` exists
- `suggested_tag` is not empty
- summary fields are present

### 3. Benchmark Candidate

Target file:

- `finish_outputs/candidate_exports/benchmark_candidate.json`

Checks to consider:

- file exists
- JSON parses
- `id` exists
- `item_type` exists
- `tags` is present
- prompt text is non-empty
- candidate wording is not too session-local
- ID style looks stable enough for long-term regression use

---

## First-Pass Heuristics

The first version should stay simple and explainable.

Recommended heuristic classes:

1. Presence checks

- file exists
- required field exists

2. Length checks

- text is not too short
- summary is not empty

3. Style checks

- benchmark ID matches a project-friendly pattern
- suggested tag is not placeholder-like
- wording does not overfit to one transient bench moment

4. Review hints

- warn if the candidate looks too raw
- warn if the candidate probably needs manual rewriting

---

## What This Should Not Do

This quality check should not:

- auto-rewrite candidate files
- auto-promote candidates
- auto-reject important evidence
- block the user from manual review

It should only:

- summarize quality signals
- highlight likely problems
- help prioritize human review effort

---

## Suggested JSON Shape

First-pass shape could look like:

```json
{
  "session_id": "bench-am-01",
  "overall_passed": false,
  "case_candidate": {
    "exists": true,
    "passed": true,
    "warnings": []
  },
  "log_candidate": {
    "exists": true,
    "passed": true,
    "warnings": []
  },
  "benchmark_candidate": {
    "exists": true,
    "passed": false,
    "warnings": [
      "candidate id looks too session-local",
      "prompt text is too short"
    ]
  },
  "next_action": "review benchmark candidate wording before promotion"
}
```

This keeps the output easy for both humans and future automation.

---

## Suggested Markdown Shape

The Markdown output should answer:

- which candidate failed first-pass quality
- why it failed
- whether manual review is still worth doing
- what the reviewer should inspect first

Ideal sections:

- overall verdict
- case candidate
- log candidate
- benchmark candidate
- next action

---

## Best Integration Point

Recommended future flow:

1. `finish-real-bench`
2. `candidate-quality-check`
3. `review-finish-candidates`
4. `promote-finish-candidates`

That keeps the new step low-risk and useful.

---

## Definition Of Done

This task is complete when:

- the command exists
- it reads the three candidate exports
- it writes JSON and Markdown summaries
- it adds at least one test covering pass/fail behavior
- it does not mutate candidate files

---

## Related Docs

- `docs/real_bench_to_formal_merge_map.md`
- `docs/real_bench_first_day_plan.md`
- `docs/formal_merge_quickstart.md`
