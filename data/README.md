# Data Directory

This directory contains the normalized training assets for the Industrial Embedded Dev Agent MVP.

The current active baseline is `i.MX8MP A53 + RPMsg + M7 + EtherCAT Dynamic Profile / robot6 position mode / Fake Harness`. Early CANopen dual-drive content is kept only as historical reference and is not the primary training line.

## Layout

- `benchmark/`
  Benchmark seeds in JSONL format, including current EtherCAT profile knowledge QA, log attribution, and tool-safety subsets.
- `taxonomy/`
  Tag definitions and category notes used to constrain structured outputs.
- `materials/`
  Material indexes and seed asset inventories for current profile/PDO/topology/fake-harness documents, reports, cases, and scripts.

## Notes

- The current content is mirrored from `准备产物/` and should be treated as the canonical machine-readable version.
- This directory is intended to become the stable interface between raw project notes and later retrieval, evaluation, and agent workflows.
- Version `v1` filenames are retained for compatibility, while the content has moved to the V2 EtherCAT Dynamic Profile baseline.
