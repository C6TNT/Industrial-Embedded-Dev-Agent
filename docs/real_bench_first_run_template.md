# Real Bench First-Run Record Template

Use this template the first time you run the real bench in `real` mode.

It is intentionally biased toward `SCRIPT-004` and read-only evidence gathering.

## Basic Info

- Date:
- Operator:
- Bench location:
- Board / platform:
- Servo model:
- Motor model:
- Project branch / commit:
- Current tool execution mode:

## Session Goal

- What do you want to verify first?
- Why is this session being run now?
- What is the lowest-risk success criterion for today?

## Environment State

### CLI / Repository

- `python -m industrial_embedded_dev_agent overview`:
- `python -m industrial_embedded_dev_agent tools mode`:
- `python -m industrial_embedded_dev_agent tools doctor`:

### WSL Runtime

- `python3 --version`:
- `ls -l /home/librobot.so.1.0.0`:
- `file /home/librobot.so.1.0.0`:

### Physical Bench

- Servo power status:
- Motor wiring checked:
- Communication chain checked:
- Emergency stop / stop condition confirmed:
- Any abnormal sound / motion / warning before script run:

## Planned Command

### Plan Step

Command:

```powershell
python -m industrial_embedded_dev_agent tools plan "<read-only axis status request>"
```

Suggested request text:

```text
Read axis0/axis1 statusword, error code, and encoder values so I can confirm the link is alive.
```

Observed result:

- tool_id:
- risk_level:
- allowed_to_execute:
- requires_confirmation:
- reason:

### Execute Step

Command:

```powershell
python -m industrial_embedded_dev_agent tools run "<read-only axis status request>" --execute
```

Observed result:

- returncode:
- transport_state:
- poll_count:
- next_action:

## Parsed Snapshot

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

## Raw Evidence

- stdout path or pasted excerpt:
- stderr path or pasted excerpt:
- parsed JSON path or pasted excerpt:
- vendor tool screenshot path:
- SDO readback notes path:

## Cross-Check

### Vendor Tool Comparison

- Does vendor-tool state match script readback?
- If not, what mismatches exist?

### SDO / Object Dictionary Comparison

- Which objects were checked?
- Did the values match the script output?
- Any suspicious differences?

## First Conclusion

- Is the transport readable?
- Are both axes returning stable data?
- Is there any unexpected non-zero error code?
- Is the bench safe to continue with more probing?

## Risk Decision

- Continue only with read-only probing
- Pause and fix environment
- Escalate to manual engineering review

Selected decision:

Reason:

## Follow-Up Actions

1.
2.
3.

## Attachments

- Screenshot 1:
- Screenshot 2:
- Log file:
- Notes file:
