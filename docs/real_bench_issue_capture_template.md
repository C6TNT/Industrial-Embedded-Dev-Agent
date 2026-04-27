# Real Bench Issue Capture Template

Use this template when the first real-bench session does not go as expected.

It is designed for early-stage issue capture, not for polished reporting.  
The priority is to preserve the evidence chain while the bench state is still fresh.

## 1. Basic Context

- Date:
- Operator:
- Bench location:
- Board / platform:
- Servo drive:
- Motor:
- Current branch / commit:
- Current execution mode:

## 2. Trigger Point

- Which command or action exposed the issue?
- Was this during `plan`, `doctor`, or `run --execute`?
- Which tool was involved?
- Was the request read-only, low-risk execution, or a blocked high-risk request?

## 3. Exact User Intent

Write the original request or operation intent here:

```text
<paste the exact request or describe the exact manual action>
```

## 4. Command Record

### Command

```powershell
<paste the exact command>
```

### CLI Result

- tool_id:
- risk_level:
- allowed_to_execute:
- requires_confirmation:
- should_refuse:
- returncode:

## 5. Observed Symptom

Describe the first visible symptom as literally as possible.

Examples:

- link unreadable
- `OpenRpmsg` failed
- non-zero drive error code
- axis readback unstable
- vendor tool and script output disagree
- unexpected motion / sound / alarm

Observed symptom:

- 

## 6. Structured Snapshot

### Transport

- execution_mode:
- transport_state:
- `OpenRpmsg` return:
- poll_count:

### Axis 0

- error_code:
- status_code:
- axis_status:
- encoder:
- axis_health:

### Axis 1

- error_code:
- status_code:
- axis_status:
- encoder:
- axis_health:

## 7. Raw Evidence

- stdout excerpt:
- stderr excerpt:
- parsed JSON excerpt:
- vendor tool screenshot:
- dynamic profile query notes:
- oscilloscope / bus capture path:

## 8. Mismatch Check

If there was a mismatch, write it explicitly.

### Script vs Vendor Tool

- Match or mismatch:
- Details:

### Script vs Dynamic Runtime Readback

- Match or mismatch:
- Details:

### Script vs Expected Historical Baseline

- Match or mismatch:
- Details:

## 9. Risk Assessment

Choose the most accurate state:

- low-risk readback issue only
- environment recovery issue
- communication chain issue
- drive-side fault indication
- unsafe to continue

Selected assessment:

- 

Why:

- 

## 10. Immediate Containment Action

What was done immediately after the issue was seen?

Examples:

- stop execution
- keep only read-only probing
- power down drive
- switch back to manual vendor-tool inspection
- stop using script path and collect environment evidence first

Action taken:

- 

## 11. What Was Already Ruled Out

List checks already completed so others do not repeat them blindly.

- 
- 
- 

## 12. What Is Still Unknown

List the top unknowns that block the next decision.

- 
- 
- 

## 13. Recommended Next Step

Pick the next action that reduces uncertainty with the least additional risk.

- continue with read-only validation
- fix environment first
- compare against vendor tool
- compare against dynamic profile/runtime readback
- escalate for manual engineering review

Selected next step:

- 

Reason:

- 

## 14. Attachments

- screenshot 1:
- screenshot 2:
- raw log file:
- parsed output file:
- additional notes:
