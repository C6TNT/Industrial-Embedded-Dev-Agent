# Real Bench Session Review Template

Use this template after a real bench session ends.

It is meant to turn a single bench session into reusable project knowledge, not just a personal memory dump.

## 1. Session Summary

- Date:
- Operator:
- Bench location:
- Board / platform:
- Servo drive:
- Motor:
- Branch / commit:
- Session duration:

## 2. Session Goal

- What was the planned goal before the session started?
- Was the goal achieved fully, partially, or not at all?

Goal statement:

- 

Outcome:

- achieved
- partially achieved
- not achieved

## 3. What Was Actually Done

List the concrete actions that were performed in order.

1.
2.
3.
4.

## 4. Commands And Tools Used

- `tools mode`:
- `tools doctor`:
- `tools plan`:
- `tools run --execute`:
- vendor tool used:
- dynamic profile/runtime checks used:

## 5. Confirmed Facts

Only write items that are now supported by evidence.

Examples:

- transport path is readable
- axis0 readback is stable
- vendor tool agrees with script on status code
- current runtime path is correct

Confirmed facts:

- 
- 
- 

## 6. Unconfirmed Assumptions

Write the items that still feel likely but are not yet proven.

Examples:

- drive parameter is not latching
- issue is axis-specific rather than link-wide
- mismatch comes from runtime environment rather than drive state

Unconfirmed assumptions:

- 
- 
- 

## 7. Key Evidence Produced

- raw stdout/stderr:
- parsed JSON:
- vendor tool screenshots:
- dynamic profile query notes:
- oscilloscope / bus capture:
- additional logs:

## 8. Risk Boundary Status

State clearly what remained within the intended safety boundary.

### Stayed Within Boundary

- read-only probing only
- low-risk collection only
- no online PDO remapping
- no firmware flashing
- no automatic high-risk motion execution

Actual session boundary:

- 

### Boundary Concerns

- Did anything feel unsafe?
- Was there any point where continuing would have been risky?

Notes:

- 

## 9. Main Findings

Summarize the 1-3 highest-value findings from the session.

1.
2.
3.

## 10. Main Blockers

What prevented deeper progress?

- environment blocker:
- hardware blocker:
- communication blocker:
- evidence blocker:
- time blocker:

## 11. Best Next Step

Choose the single next action that reduces uncertainty the most with the least extra risk.

Selected next step:

- 

Why this is next:

- 

## 12. Lower-Priority Follow-Ups

List useful but non-critical follow-ups.

1.
2.
3.

## 13. What Should Be Added To The Project

Record anything that should feed back into the repository itself.

Examples:

- add a new benchmark case
- add a new issue case
- expand a tool parser
- update the readiness checklist
- archive a new log sample

Repository follow-up items:

- 
- 
- 

## 14. Reuse Value

How should this session help future work?

- reusable command:
- reusable log sample:
- reusable issue pattern:
- reusable checklist update:

## 15. Final Session Verdict

Pick the best description:

- clean baseline established
- environment partially recovered
- read-only validation succeeded
- mismatch found, needs follow-up
- unsafe to continue without manual review

Selected verdict:

- 

Short explanation:

- 
