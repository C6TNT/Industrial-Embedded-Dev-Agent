# Document Chunks

This directory stores normalized chunk datasets used by the first-pass RAG pipeline.

## Files

- `doc_chunks_v1.jsonl`
  Chunked text extracted from curated project documents, worklogs, manuals, and logs.

## Current Policy

- Prefer high-value engineering sources such as project plan notes, learning summaries, worklogs, and key manuals.
- Skip duplicated workspaces and oversized demo bundles.
- Keep chunk records traceable with `source_id`, `source_path`, and `ordinal`.
