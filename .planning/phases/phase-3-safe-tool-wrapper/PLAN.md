# Phase 3 Plan: Safe Tool Wrapper

## Goal

Expose useful existing scripts as guarded tools.

## Tasks

1. Classify existing `D:\桌面\test\mix_protocol\tools` scripts by risk:
   - `offline_ok`;
   - `board_required`;
   - `robot_motion_required`;
   - `io_required`;
   - `firmware_required`.
2. Allow automatic execution only for offline scripts.
3. For hardware-facing scripts, generate an audit/checklist instead of running them.
4. Reuse existing fake harness and replay tools; do not rewrite harness loops.

## Completion Criteria

- Tool metadata is visible to the Agent.
- Safe tools can run through one wrapper.
- Dangerous tools are blocked by default.

## Current Implementation

- `spindle tools material-tools`
- `spindle tools material-tool-plan <tool-path>`
- `spindle tools material-run-tool <tool-path>`

## Safety

No `ssh`, `scp`, `reboot`, `start-bus`, `stop-bus`, `0x86`, `0x41F1`, motion, IO output, remoteproc, or firmware actions.
