# Document Chunks

This directory stores normalized chunk datasets used by the first-pass RAG pipeline.

## Files

- `doc_chunks_v1.jsonl`
  Chunked text extracted from the current active material set: project plan, material index, label taxonomy, and EtherCAT Dynamic Profile baseline notes.

## Current Policy

- Prefer current engineering sources around A53/RPMsg/M7/EtherCAT profile, PDO layout, robot6 position mode, dynamic runtime takeover, IO profile, and Fake Harness.
- Skip duplicated workspaces, oversized demo bundles, and historical CANopen-only documents unless they are explicitly reintroduced as archive material.
- Keep chunk records traceable with `source_id`, `source_path`, and `ordinal`.
