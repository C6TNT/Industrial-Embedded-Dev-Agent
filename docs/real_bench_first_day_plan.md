# Real Bench First Day Plan

## Goal

This is a practical day-one plan for the first day back on the real bench.

It assumes:

- the hardware is finally available again
- you want real evidence, not demo-only validation
- you still want to keep the first day conservative

The operating rule is simple:

- first recover a readable environment
- then capture one clean read-only evidence pack
- only after that decide whether a second session is worth planning

---

## Morning Plan

### Block 1: Environment Recovery

Run:

```bash
python -m industrial_embedded_dev_agent overview
python -m industrial_embedded_dev_agent tools use-real
python -m industrial_embedded_dev_agent tools mode
python -m industrial_embedded_dev_agent tools doctor
wsl.exe bash -lc "python3 --version"
wsl.exe bash -lc "ls -l /home/librobot.so.1.0.0"
wsl.exe bash -lc "file /home/librobot.so.1.0.0"
```

Success criteria:

- CLI works
- execution mode is not `stub`
- runtime path exists
- WSL side basic checks are normal

If this block fails, do not continue into servo debugging yet.
Use the morning for environment repair only.

### Block 2: Physical Bench Sanity Check

Manual checks:

- servo power state
- motor wiring
- communication chain
- board state
- no unexpected warning, motion, or unsafe condition

Success criteria:

- the bench looks physically safe enough for a read-only run

If not, stop here.

### Block 3: Create The Session Container

Run:

```bash
python -m industrial_embedded_dev_agent tools prep-real-bench --session-id <session-id> --label "<label>"
```

Expected outputs:

- prep bundle
- `doctor_snapshot.json`
- `plan_seed.json`

This is the point where the day becomes traceable and reviewable.

### Block 4: First Read-Only Capture

Run:

```bash
python -m industrial_embedded_dev_agent tools kickoff-real-bench "reports/real_bench_prep/<session-id>/plan_seed.json" --render-all --execute
```

Success criteria:

- transport is readable
- `SCRIPT-004` returns structured output
- both axes return stable readback
- no unexpected error code appears

If this fails, the morning goal becomes:

- collect failure evidence
- stop escalation
- do not move into deeper probes

---

## Afternoon Plan

### Block 5: Cross-Check And Interpret

Compare the read-only result with:

- vendor tool readback
- SDO / object dictionary readback
- any known-good historical notes

Goal:

- decide whether the script output is trustworthy as a bench baseline

If there is a strong mismatch, stop and log it as an issue rather than increasing risk.

### Block 6: Close The Session Properly

Run:

```bash
python -m industrial_embedded_dev_agent tools finish-real-bench --session-id <session-id>
```

Expected outputs:

- `session_bundle.md`
- `final_summary.json`
- `case_candidate.md`
- `log_candidate.json`
- `benchmark_candidate.json`

This is the step that turns the first real bench run into reusable project assets.

### Block 7: Decide What Today Was

By the end of the first day, classify the session as one of these:

1. environment recovery only
2. readable baseline established
3. readable baseline plus trustworthy cross-check

For day one, outcome 2 is already a good result.
Outcome 3 is excellent.

You do not need risky motion-path probing on day one to call the day successful.

---

## What Not To Do On Day One

Avoid these on the first day unless there is an unusually strong reason and manual engineering agreement:

- RPDO remapping
- control-word rewriting beyond the current safe policy
- acceleration/deceleration write verification
- motion-driving probe scripts
- any L2 execution shortcut

The first day should prioritize:

- readability
- traceability
- confidence

not aggressive automation.

---

## Recommended End-Of-Day Outputs

If the session produced useful evidence, make sure these exist before leaving:

- prep bundle
- kickoff outputs
- finish outputs
- first-run notes
- vendor / SDO comparison notes

If possible, also run:

```bash
python -m industrial_embedded_dev_agent tools review-finish-candidates --session-id <session-id>
```

That gives you a cleaner next-day starting point.

---

## Day-One Success Definition

A successful first day does not mean:

- motion was commanded
- deep probe scripts were executed
- canonical data was updated

A successful first day means:

- real mode was restored
- one read-only bench pack was captured
- evidence was archived cleanly
- the next session can begin from facts instead of guesswork

---

## Related Docs

- `docs/real_bench_quickstart.md`
- `docs/real_bench_readiness_checklist.md`
- `docs/real_bench_first_run_template.md`
- `docs/real_bench_to_formal_merge_map.md`
- `docs/formal_merge_quickstart.md`
