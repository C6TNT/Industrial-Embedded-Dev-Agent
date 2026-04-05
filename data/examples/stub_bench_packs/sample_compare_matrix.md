# Stub Sample Compare Matrix

## Purpose

This matrix defines the recommended offline compare drills for the curated stub bench-pack sample set.

Use it when you want a stable regression target for:

- `tools compare-pack`
- parser verification
- session bundle latest-change summaries
- issue / review draft rehearsal

## Compare Matrix

| Left Sample | Right Sample | Primary intent | Expected changed axes | Expected key difference |
|---|---|---|---|---|
| `sample_nominal.json` | `sample_axis1_fault.json` | rehearse single-axis anomaly detection | `axis1` | `error_code: 0 -> 16`, `status_code: 33 -> 144`, `axis_status: 4097 -> 9473`, encoder collapses from a healthy baseline to `8` |
| `sample_nominal.json` | `sample_encoder_stall.json` | rehearse stagnant feedback detection | `axis0`, `axis1` | transport stays readable, but encoder values stop reflecting the nominal incremental pattern |
| `sample_nominal.json` | `sample_open_rpmsg_fail.json` | rehearse transport degradation detection | `axis0`, `axis1` | `OpenRpmsg: 0 -> -1`, transport changes from `readable` to `unverified`, axis readback degrades to zeros |
| `sample_encoder_stall.json` | `sample_axis1_fault.json` | distinguish feedback stagnation from true single-axis fault | `axis1` | `axis1` shifts from flat-but-readable to explicit abnormal servo state |
| `sample_axis1_fault.json` | `sample_open_rpmsg_fail.json` | distinguish servo anomaly from transport-layer failure | `axis0`, `axis1` | readable single-axis fault becomes transport-wide degradation |

## Recommended Commands

```powershell
python -m industrial_embedded_dev_agent tools compare-pack "D:\桌面\Industrial Embedded Dev Agent\data\examples\stub_bench_packs\sample_nominal.json" "D:\桌面\Industrial Embedded Dev Agent\data\examples\stub_bench_packs\sample_axis1_fault.json"
python -m industrial_embedded_dev_agent tools compare-pack "D:\桌面\Industrial Embedded Dev Agent\data\examples\stub_bench_packs\sample_nominal.json" "D:\桌面\Industrial Embedded Dev Agent\data\examples\stub_bench_packs\sample_encoder_stall.json"
python -m industrial_embedded_dev_agent tools compare-pack "D:\桌面\Industrial Embedded Dev Agent\data\examples\stub_bench_packs\sample_nominal.json" "D:\桌面\Industrial Embedded Dev Agent\data\examples\stub_bench_packs\sample_open_rpmsg_fail.json"
```

## Review Checklist

- Confirm the compare output marks the expected changed axes.
- Confirm the compare output names the expected transport or servo-layer change.
- Confirm the generated Markdown diff is still readable enough to paste into an issue or session review.
- If the observed diff drifts from this matrix, either the parser regressed or the curated sample needs to be updated deliberately.
