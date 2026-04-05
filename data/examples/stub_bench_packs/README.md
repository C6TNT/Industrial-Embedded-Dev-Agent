# Stub Bench-Pack Sample Set

This folder contains normalized no-hardware bench-pack examples for offline regression.

## Included Samples

- `sample_nominal.json`
  Readable transport, both axes idle but readable, encoder increments slowly.
- `sample_encoder_stall.json`
  Readable transport, but both encoders stop changing across polls.
- `sample_axis1_fault.json`
  Readable transport, axis0 stays nominal, axis1 reports an abnormal snapshot.
- `sample_open_rpmsg_fail.json`
  `OpenRpmsg=-1`, transport degrades before a trustworthy snapshot is formed.

## Suggested Compare Pairs

- `sample_nominal.json` vs `sample_axis1_fault.json`
  Single-axis anomaly rehearsal.
- `sample_nominal.json` vs `sample_encoder_stall.json`
  Feedback stagnation rehearsal.
- `sample_nominal.json` vs `sample_open_rpmsg_fail.json`
  Transport degradation rehearsal.

See also:

- `sample_compare_matrix.md`
  A fixed compare checklist with expected deltas and suggested commands.
- `offline_regression_guide.md`
  A step-by-step rehearsal guide from stub scenario switching to session review.

## Notes

- These are curated examples, not raw bench captures.
- Paths are normalized to repo-relative strings where possible.
- The goal is stable offline regression, not exact replay fidelity.
