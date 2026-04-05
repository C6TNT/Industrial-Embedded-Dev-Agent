from __future__ import annotations

import ctypes
import json
from pathlib import Path


DEFAULT_STUB_SCENARIO = "nominal"
STUB_SCENARIOS = {
    "nominal": "Readable transport with stable idle snapshots on both axes.",
    "encoder_stall": "Transport is readable, but encoder feedback stops changing on both axes.",
    "axis1_fault": "Transport is readable, axis1 reports a non-zero error code and abnormal status.",
    "open_rpmsg_fail": "Transport initialization degrades immediately and OpenRpmsg returns a failure code.",
}


class _StubFunction:
    def __init__(self, callback):
        self._callback = callback
        self.argtypes = None
        self.restype = None

    def __call__(self, *args):
        return self._callback(*args)


class FakeLibrobot:
    def __init__(self, scenario: str = DEFAULT_STUB_SCENARIO) -> None:
        self._scenario = scenario if scenario in STUB_SCENARIOS else DEFAULT_STUB_SCENARIO
        self._error_code = [0, 0]
        self._status_code = [33, 33]
        self._axis_status = [4097, 4097]
        self._encoder = [1000, 2000]
        self._target_vel = [0, 0]
        self._direction = [0, 0]
        self._atype = [0, 0]
        self._single_transfer = [0, 0]
        self._accel = [100, 100]
        self._decel = [100, 100]
        self._axis_enable = [0, 0]

        self.OpenRpmsg = _StubFunction(self._open_rpmsg)
        self.BusCmd_InitBus = _StubFunction(self._init_bus)
        self.Weld_Direct_GetErrorCode = _StubFunction(self._get_error_code)
        self.Weld_Direct_GetStatusCode = _StubFunction(self._get_status_code)
        self.Weld_Direct_GetAxisStatus = _StubFunction(self._get_axis_status)
        self.Weld_Direct_GetEncoder = _StubFunction(self._get_encoder)
        self.Weld_Direct_SetTargetVel = _StubFunction(self._set_target_vel)
        self.Weld_Direct_SetDirection = _StubFunction(self._set_direction)
        self.Weld_Direct_SetAType = _StubFunction(self._set_atype)
        self.Weld_Direct_SetSingleTransper = _StubFunction(self._set_single_transfer)
        self.Weld_Direct_SetAccel = _StubFunction(self._set_accel)
        self.Weld_Direct_SetDecel = _StubFunction(self._set_decel)
        self.Weld_Direct_SetAxisEnable = _StubFunction(self._set_axis_enable)
        self.Weld_Direct_GetAType = _StubFunction(self._get_atype)
        self.Weld_Direct_GetAxisEnable = _StubFunction(self._get_axis_enable)
        self.Weld_Direct_GetAccel = _StubFunction(self._get_accel)
        self.Weld_Direct_GetDecel = _StubFunction(self._get_decel)
        self._apply_scenario()

    def _apply_scenario(self) -> None:
        if self._scenario == "encoder_stall":
            self._encoder = [1500, 2500]
        elif self._scenario == "axis1_fault":
            self._error_code[1] = 16
            self._status_code[1] = 144
            self._axis_status[1] = 9473
            self._encoder[1] = 0
        elif self._scenario == "open_rpmsg_fail":
            self._status_code = [0, 0]
            self._axis_status = [0, 0]
            self._encoder = [0, 0]

    def _axis(self, axis: int) -> int:
        return 0 if axis <= 0 else 1

    def _write_ptr(self, pointer, value: int) -> None:
        casted = ctypes.cast(pointer, ctypes.POINTER(ctypes.c_int32))
        casted[0] = int(value)

    def _open_rpmsg(self, handle_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            return -1
        if hasattr(handle_pointer, "_obj"):
            handle_pointer._obj.InitBusEnable = False
        return 0

    def _init_bus(self, handle_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            return -1
        if hasattr(handle_pointer, "_obj"):
            handle_pointer._obj.InitBusEnable = True
        return 0

    def _get_error_code(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._error_code[self._axis(axis)])
        return 0

    def _get_status_code(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._status_code[self._axis(axis)])
        return 0

    def _get_axis_status(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._axis_status[self._axis(axis)])
        return 0

    def _get_encoder(self, handle_pointer, axis: int, value_pointer) -> int:
        idx = self._axis(axis)
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        if self._scenario != "encoder_stall":
            self._encoder[idx] += 25 if self._axis_enable[idx] else 1
        self._write_ptr(value_pointer, self._encoder[idx])
        return 0

    def _set_target_vel(self, handle_pointer, axis: int, value: int) -> int:
        idx = self._axis(axis)
        self._target_vel[idx] = int(value)
        self._status_code[idx] = 39 if value else 33
        return 0

    def _set_direction(self, handle_pointer, axis: int, value: int) -> int:
        self._direction[self._axis(axis)] = int(value)
        return 0

    def _set_atype(self, handle_pointer, axis: int, value: int) -> int:
        self._atype[self._axis(axis)] = int(value)
        return 0

    def _set_single_transfer(self, handle_pointer, axis: int, value: int) -> int:
        self._single_transfer[self._axis(axis)] = int(value)
        return 0

    def _set_accel(self, handle_pointer, axis: int, value: int) -> int:
        self._accel[self._axis(axis)] = int(value)
        return 0

    def _set_decel(self, handle_pointer, axis: int, value: int) -> int:
        self._decel[self._axis(axis)] = int(value)
        return 0

    def _set_axis_enable(self, handle_pointer, axis: int, value: int) -> int:
        idx = self._axis(axis)
        self._axis_enable[idx] = int(value)
        self._axis_status[idx] = 8193 if value else 4097
        return 0

    def _get_atype(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._atype[self._axis(axis)])
        return 0

    def _get_axis_enable(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._axis_enable[self._axis(axis)])
        return 0

    def _get_accel(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._accel[self._axis(axis)])
        return 0

    def _get_decel(self, handle_pointer, axis: int, value_pointer) -> int:
        if self._scenario == "open_rpmsg_fail":
            self._write_ptr(value_pointer, 0)
            return -1
        self._write_ptr(value_pointer, self._decel[self._axis(axis)])
        return 0


def load_stub_profile(profile_path: Path) -> dict[str, str]:
    if not profile_path.exists():
        return {"scenario": DEFAULT_STUB_SCENARIO}
    try:
        payload = json.loads(profile_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {"scenario": DEFAULT_STUB_SCENARIO}
    scenario = str(payload.get("scenario", DEFAULT_STUB_SCENARIO))
    if scenario not in STUB_SCENARIOS:
        scenario = DEFAULT_STUB_SCENARIO
    return {"scenario": scenario}


def patch_ctypes_cdll(target_path: str = "/home/librobot.so.1.0.0", *, scenario: str = DEFAULT_STUB_SCENARIO) -> None:
    original_cdll = ctypes.CDLL

    def patched(path, *args, **kwargs):
        if path == target_path:
            return FakeLibrobot(scenario=scenario)
        return original_cdll(path, *args, **kwargs)

    ctypes.CDLL = patched
