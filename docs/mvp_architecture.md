# MVP Architecture

## Goal

Build a minimal but expandable engineering baseline for the Industrial Embedded Dev Agent project.

## Scope

The MVP focuses on three stable interfaces:

1. `data/benchmark`
   Structured evaluation seeds.
2. `data/taxonomy`
   Tag constraints for issue typing, causes, actions, and risk levels.
3. `data/materials`
   A curated material index that points to high-value source assets.

## Package Layout

- `src/industrial_embedded_dev_agent/config.py`
  Repository path discovery and data-path helpers.
- `src/industrial_embedded_dev_agent/models.py`
  Lightweight dataclasses for benchmark items and label groups.
- `src/industrial_embedded_dev_agent/benchmarks.py`
  JSONL loading, filtering, and summary generation.
- `src/industrial_embedded_dev_agent/taxonomy.py`
  Tag extraction and taxonomy summaries.
- `src/industrial_embedded_dev_agent/datasets.py`
  Project-level dataset overview helpers.
- `src/industrial_embedded_dev_agent/cli.py`
  Minimal command-line interface.

## Near-Term Next Steps

1. Add dataset ingestion for extracted doc chunks and structured log samples.
2. Add benchmark validators and simple scoring logic.
3. Add retrieval and case-reuse pipelines on top of the normalized `data/` interface.
