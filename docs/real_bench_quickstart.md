# Real Bench Quickstart

## Goal

This is the shortest safe path for the first day back on the real bench.

Use it when you want to:

- leave stub mode
- confirm the real runtime is readable
- capture one safe read-only bench evidence pack
- leave the session with a clean next step

This guide is intentionally conservative.
It is biased toward read-only validation first.

---

## Minimal Safe Path

Run in this order:

```bash
python -m industrial_embedded_dev_agent tools use-real
python -m industrial_embedded_dev_agent tools doctor
wsl.exe bash -lc "python3 --version"
wsl.exe bash -lc "ls -l /home/librobot.so.1.0.0"
wsl.exe bash -lc "file /home/librobot.so.1.0.0"
python -m industrial_embedded_dev_agent tools prep-real-bench --session-id <session-id> --label "<label>"
python -m industrial_embedded_dev_agent tools kickoff-real-bench "reports/real_bench_prep/<session-id>/plan_seed.json" --render-all --execute
python -m industrial_embedded_dev_agent tools finish-real-bench --session-id <session-id>
```

If the environment is not ready, stop before kickoff.

---

## What To Confirm Before Execution

Before touching the bench, confirm all of these:

- `tools doctor` shows `wsl_available=true`
- execution mode is not `stub`
- `/home/librobot.so.1.0.0` exists
- servo power state is known
- communication chain is connected
- no unsafe motion condition is present

If any of those are unclear, do not run the script yet.

---

## First Command You Should Trust

For the first real session, trust only the low-risk read-only path:

- `SCRIPT-004`

Expected characteristics:

- tool selection should stay read-only
- the request should not cross the L2 boundary
- parsed output should include transport and axis summaries

Do not begin with:

- online PDO remapping or dynamic output-path switching
- control-word rewriting
- acceleration/deceleration write probes
- any motion-driving script

---

## First Output Files To Look At

After kickoff, inspect:

1. `reports/real_bench_prep/<session-id>/kickoff_outputs/bench_pack.json`
2. `reports/real_bench_prep/<session-id>/kickoff_outputs/first_run.md`
3. `reports/real_bench_prep/<session-id>/kickoff_outputs/session_review.md`
4. `reports/real_bench_prep/<session-id>/kickoff_outputs/run_summary.md`

You want to answer:

- is transport readable?
- do both axes return stable data?
- is there any non-zero unexpected error code?
- does the script readback agree with dynamic profile query and the stable report baseline?

---

## Stop Conditions

Stop immediately if:

- execution mode is still `real_unavailable`
- `OpenRpmsg` fails
- a non-zero drive error appears unexpectedly
- vendor tool readback strongly disagrees with script readback
- power / communication state is unclear

If that happens, treat the session as environment recovery, not servo debugging.

---

## Recommended Session Close-Out

If the first read-only run is valid, finish the session with:

```bash
python -m industrial_embedded_dev_agent tools finish-real-bench --session-id <session-id>
```

That creates:

- session bundle
- final summary
- candidate exports for case / log / benchmark

Those outputs become the first bridge from real bench evidence into the formal merge pipeline.

---

## What To Read Next

After the first successful real-bench session, continue with:

1. `docs/real_bench_first_day_plan.md`
2. `docs/real_bench_to_formal_merge_map.md`
3. `docs/formal_merge_quickstart.md`
4. `docs/manual_canonical_merge_guide.md`

That gives you the shortest path from:

- real bench evidence

to:

- candidate review
- pending promotion
- formal merge preparation
- eventual manual canonical merge
