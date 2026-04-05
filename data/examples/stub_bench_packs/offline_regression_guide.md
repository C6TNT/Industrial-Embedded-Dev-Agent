# Offline Regression Guide

## Goal

This guide walks through the full offline rehearsal loop for the no-hardware tool chain:

1. switch stub scenario
2. capture bench-pack evidence
3. compare packs
4. render a session bundle
5. check the fixed sample set

It is designed for use before real bench access is available.

## 1. Inspect Available Stub Scenarios

```powershell
python -m industrial_embedded_dev_agent tools stub-scenarios
python -m industrial_embedded_dev_agent tools doctor
```

What to confirm:

- `stub_scenario` is visible in `tools doctor`
- current mode is `stub`

## 2. Capture A Nominal Baseline

```powershell
python -m industrial_embedded_dev_agent tools setup-stub --scenario nominal
python -m industrial_embedded_dev_agent tools bench-pack "run tmp_probe_can_heartbeat.py and capture a nominal readonly snapshot" --session-id offline-drill --label "Offline drill"
```

Expected shape:

- `OpenRpmsg=0`
- `transport_state=readable`
- both axes look idle but readable

## 3. Capture A Contrast Scenario

Pick one branch below.

### Option A: Single-Axis Fault

```powershell
python -m industrial_embedded_dev_agent tools setup-stub --scenario axis1_fault
python -m industrial_embedded_dev_agent tools bench-pack "run tmp_probe_can_heartbeat.py and capture an axis1 fault snapshot" --session-id offline-drill --label "Offline drill"
```

Expected shape:

- `axis0` stays nominal
- `axis1` shows non-zero error code and abnormal status

### Option B: Encoder Stall

```powershell
python -m industrial_embedded_dev_agent tools setup-stub --scenario encoder_stall
python -m industrial_embedded_dev_agent tools bench-pack "run tmp_probe_can_heartbeat.py and capture an encoder stall readonly snapshot" --session-id offline-drill --label "Offline drill"
```

Expected shape:

- transport still readable
- encoder feedback no longer reflects the nominal incremental pattern

### Option C: RPMsg Degradation

```powershell
python -m industrial_embedded_dev_agent tools setup-stub --scenario open_rpmsg_fail
python -m industrial_embedded_dev_agent tools bench-pack "run tmp_probe_can_heartbeat.py and capture an OpenRpmsg failure snapshot" --session-id offline-drill --label "Offline drill"
```

Expected shape:

- `OpenRpmsg=-1`
- `transport_state=unverified`
- readback degrades across both axes

## 4. Compare The Latest Two Packs In One Session

```powershell
python -m industrial_embedded_dev_agent tools compare-pack --session-id offline-drill
```

What to confirm:

- changed axes match the chosen scenario
- transport changes are surfaced when expected
- observations mention the main delta

## 5. Render A Session Review

```powershell
python -m industrial_embedded_dev_agent tools render-session offline-drill
```

What to confirm:

- `Latest Change Snapshot` appears in the rendered Markdown
- the summary matches the compare output
- the session verdict still makes sense for an offline drill

## 6. Validate Against The Curated Sample Set

The repository also includes stable canned examples under:

- `data/examples/stub_bench_packs/`

Recommended checks:

```powershell
python -m industrial_embedded_dev_agent tools compare-pack "data/examples/stub_bench_packs/sample_nominal.json" "data/examples/stub_bench_packs/sample_axis1_fault.json"
python -m industrial_embedded_dev_agent tools compare-pack "data/examples/stub_bench_packs/sample_nominal.json" "data/examples/stub_bench_packs/sample_open_rpmsg_fail.json"
python -m industrial_embedded_dev_agent benchmark run --engine rules
```

Cross-reference:

- `sample_compare_matrix.md`

## 7. Exit Criteria

An offline rehearsal is in good shape when:

- stub scenario switching is visible in `tools doctor`
- bench-pack captures remain structurally valid
- compare-pack highlights the intended delta
- render-session includes the latest change summary
- rules benchmark remains green

## 8. Transition Back To Real Bench Work

Once hardware is available again:

1. switch back with `python -m industrial_embedded_dev_agent tools use-real`
2. run `tools doctor`
3. start with a readonly probe
4. compare the real pack against a trusted baseline

The offline flow does not replace real hardware validation, but it gives a stable way to rehearse evidence capture, diffing, and review generation before that stage.
