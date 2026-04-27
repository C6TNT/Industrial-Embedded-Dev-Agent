# Real Bench Readiness Checklist

This checklist is for the first day back at the company, when the real servo drive, motor, and bench wiring are available again.

The goal is not to jump straight into risky motion commands.  
The goal is to bring the environment back from offline stub mode into a real, readable, low-risk bench state first.

## 1. Before Touching The Bench

Confirm the repository and CLI still work:

```powershell
python -m industrial_embedded_dev_agent overview
python -m industrial_embedded_dev_agent tools mode
python -m industrial_embedded_dev_agent tools doctor
```

Expected:

- the CLI starts normally
- `tools doctor` reports `wsl_available=true`
- current execution mode is visible

## 2. Leave Stub Mode

Switch the tool layer out of offline mode:

```powershell
python -m industrial_embedded_dev_agent tools use-real
python -m industrial_embedded_dev_agent tools mode
```

Expected:

- mode is no longer `stub`
- if the real runtime is not ready yet, you will see `real_unavailable`

Do not proceed to execution if you are still unsure which mode you are in.

## 3. Check The Real WSL Runtime Path

The current real-hardware path assumption is:

```text
/home/librobot.so.1.0.0
```

Run:

```powershell
python -m industrial_embedded_dev_agent tools doctor
```

Expected in the real setup:

- `stub_mode_enabled=false`
- `stub_library_present=true`
- `execution_mode=real`

If `execution_mode=real_unavailable`, stop here and fix the runtime path first.

## 4. Verify WSL Side Basics

Inside WSL, confirm:

- Python is available
- the runtime library exists
- the architecture is correct for the current WSL environment

Suggested checks:

```powershell
wsl.exe bash -lc "python3 --version"
wsl.exe bash -lc "ls -l /home/librobot.so.1.0.0"
wsl.exe bash -lc "file /home/librobot.so.1.0.0"
```

You want to rule out:

- missing file
- wrong architecture
- stale copied library

## 5. Check The Physical Bench State

Before any script execution, confirm manually:

- servo drive is powered as expected
- motor wiring is correct
- communication chain is connected
- board power-up state is known
- no leftover dangerous motion condition exists

This step stays manual on purpose.

## 6. Start With Read-Only Validation

Do not begin with write-path or motion-path scripts.

Start with:

```powershell
python -m industrial_embedded_dev_agent tools plan "先只读 query 当前 dynamic profile，采集六轴状态字、错误码和 actual_position，确认链路是不是还活着。"
python -m industrial_embedded_dev_agent tools run "先只读 query 当前 dynamic profile，采集六轴状态字、错误码和 actual_position，确认链路是不是还活着。" --execute
```

Expected:

- tool selection = `SCRIPT-004`
- risk = `L0_readonly`
- execution is allowed
- parsed output contains:
  - `transport_state`
  - `axes`
  - `axis_health`
  - `observations`

## 7. Interpret The First Snapshot

For the first real-bench snapshot, record:

- `OpenRpmsg` return value
- `poll_count`
- each axis `error_code`
- each axis `status_code`
- each axis `axis_status`
- each axis `encoder`

Focus first on these questions:

1. Is the link readable at all?
2. Are both axes returning stable readback?
3. Is there any non-zero error code?
4. Does the snapshot match what the vendor tool shows?

## 8. Cross-Check Against Dynamic Profile / Runtime Query

Once the low-risk snapshot is readable, compare it with external truth:

- vendor tool readback
- dynamic profile query output
- known robot6 topology/profile report
- known stable historical logs

This is the point where `SCRIPT-004` output becomes useful as an engineering baseline, not just as a script result.

## 9. Keep High-Risk Requests Blocked

Even on the real bench, do not skip the current risk boundary.

Still blocked by default:

- online PDO remapping or dynamic output-path switching
- control-word rewriting with automatic execution
- firmware flashing
- high-risk motion-driving probes

The current tool layer is intentionally designed so that real hardware availability does not automatically remove the L2 boundary.

## 10. Capture The First Real-Bench Evidence Pack

After the first clean read-only run, archive:

- raw tool stdout/stderr
- parsed output JSON
- vendor tool screenshot or readback notes
- any mismatch observations

This should become the first real-bench reference case for the project.

## 11. Recommended First Session Order

Recommended order for the first session back:

1. `tools use-real`
2. `tools doctor`
3. WSL path + `file /home/librobot.so.1.0.0`
4. manual bench safety check
5. `tools plan` on read-only request
6. `tools run --execute` on `SCRIPT-004`
7. compare with dynamic profile query and stable report baseline
8. archive the evidence

## 12. Stop Conditions

Stop and do not continue into deeper probing when:

- `execution_mode` is still `real_unavailable`
- `OpenRpmsg` fails
- non-zero drive error code appears unexpectedly
- vendor tool and read-only script disagree badly
- bench power / control state is unclear

If any of these happen, treat the session as environment recovery first, not as servo debugging.
