# Phase 4 Plan: Coding CLI Wrapper

## Goal

Build Spindle as a wrapper around an existing coding CLI / Agent SDK before writing a custom runtime.

## Tasks

1. Define wrapper commands:
   - `ask`;
   - `search`;
   - `plan`;
   - `audit`;
   - `check`;
   - `run-offline-tool`;
   - `summarize`.
2. Route commands through search, curated memory, and tool metadata.
3. Preserve permissions and safety boundaries.
4. Keep the wrapper swappable so a future runtime can replace the backend.

## Completion Criteria

- The wrapper can inspect files, answer with source evidence, and run offline checks.
- It refuses hardware actions automatically.
- It does not depend on a custom harness loop.

## Safety

Offline only unless the output is a plan/audit/checklist.
