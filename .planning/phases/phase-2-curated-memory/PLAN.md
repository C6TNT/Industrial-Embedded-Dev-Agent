# Phase 2 Plan: Curated Memory

## Goal

Turn stable engineering facts into small, reviewable memory entries.

## Tasks

1. Define memory item types: `CUR-*`, `LOG-*`, `CASE-*`, `TOOL-*`, `BOUNDARY-*`.
2. Draft memory from existing docs and reports, but require source paths.
3. Keep noisy/raw material searchable but outside canonical memory.
4. Add checks for missing source, weak fact text, or hardware-validation gaps.

## Completion Criteria

- New facts can be drafted without modifying canonical data.
- Each accepted fact has a source path and confidence marker.
- The Agent knows when to search raw files instead of trusting memory.

## Current Implementation

- `spindle tools material-draft-fact "<fact>" --source <material-relative-path>`

## Safety

Memory must not claim hardware validation unless the source report proves it.
