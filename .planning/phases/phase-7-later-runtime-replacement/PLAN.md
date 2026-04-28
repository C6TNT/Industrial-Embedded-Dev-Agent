# Phase 7 Plan: Later Runtime Replacement

## Goal

Only after the wrapper approach reaches real limitations, evaluate a custom runtime inspired by projects such as pi-mono.

## Tasks

1. Document wrapper limitations.
2. Identify which parts need replacement:
   - session management;
   - tool routing;
   - memory;
   - UI/TUI;
   - permissions.
3. Preserve all existing search, memory, and tool metadata.
4. Replace one subsystem at a time.

## Completion Criteria

- Runtime replacement has a written justification.
- Existing offline gates still pass.
- Hardware safety boundaries remain unchanged.

## Safety

Do not use runtime replacement as a reason to weaken hardware boundaries.
