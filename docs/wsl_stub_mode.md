# WSL No-Hardware Stub Mode

This mode lets us exercise the `ieda tools` execution path even when the real servo drive, motor, and field bus are not available.

## When To Use It

- You are away from the bench and do not have real hardware nearby.
- You want to validate `SCRIPT-004` style read-only / low-risk execution flow.
- You want to test WSL + Python + tool parsing without depending on the real `librobot.so`.

## How It Works

The repository does not modify the original probe scripts. Instead, a WSL Python wrapper intercepts:

```python
ctypes.CDLL("/home/librobot.so.1.0.0")
```

and replaces it at runtime with a pure-Python fake transport implementation.

Key files:

- `scripts/wsl/run_with_stub.py`
- `scripts/wsl/librobot_stub_runtime.py`

## Commands

Check the current execution mode:

```powershell
python -m industrial_embedded_dev_agent tools mode
python -m industrial_embedded_dev_agent tools doctor
```

Enable stub mode:

```powershell
python -m industrial_embedded_dev_agent tools setup-stub
```

List available stub scenarios:

```powershell
python -m industrial_embedded_dev_agent tools stub-scenarios
```

Enable a specific stub scenario:

```powershell
python -m industrial_embedded_dev_agent tools setup-stub --scenario nominal
python -m industrial_embedded_dev_agent tools setup-stub --scenario encoder_stall
python -m industrial_embedded_dev_agent tools setup-stub --scenario axis1_fault
python -m industrial_embedded_dev_agent tools setup-stub --scenario open_rpmsg_fail
```

Switch back to real-hardware mode:

```powershell
python -m industrial_embedded_dev_agent tools use-real
```

## Mode Meanings

- `stub`
  The tool layer runs through the local no-hardware wrapper.
- `real`
  The tool layer runs directly against `/home/librobot.so.1.0.0` in WSL.
- `real_unavailable`
  Stub mode is off, but the real WSL runtime path is not ready yet.

`tools doctor` also reports:

- `stub_scenario`
- `stub_scenario_description`

So you can tell which no-hardware branch is currently active.

## Validation Command

For offline validation, start with `SCRIPT-004`:

```powershell
python -m industrial_embedded_dev_agent tools run "只读 query 当前 dynamic profile，采集六轴状态字、错误码和 actual_position，确认链路是不是还活着。" --execute
```

Expected shape:

- `returncode = 0`
- `parsed_output.status = ok`
- `poll_count` is present
- dynamic profile / six-axis readonly snapshots are parsed into structured fields

Recommended offline scenario checks:

- `nominal`
  Baseline readable transport and stable idle snapshots.
- `encoder_stall`
  Useful for testing `compare-pack` and stagnated feedback handling.
- `axis1_fault`
  Useful for testing non-zero error code parsing and issue capture templates.
- `open_rpmsg_fail`
  Useful for validating degraded transport summaries and follow-up guidance.

## Switching Back To The Real Bench

Once you are back at the company with the real setup:

1. Run `python -m industrial_embedded_dev_agent tools use-real`
2. Prepare the real `/home/librobot.so.1.0.0` and runtime environment
3. Run `python -m industrial_embedded_dev_agent tools doctor`
4. Re-run `tools run --execute` against the real link

## Current Boundary

This mode is mainly for:

- validating `SCRIPT-004`
- validating the tool layer's `plan / execute / parse` loop

It is not a motor-motion simulator, and it does not replace real bench validation.  
High-risk `L2` scripts still remain blocked or confirmation-only.
