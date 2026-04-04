import ctypes
import os
import sys
import time


class SYS_HANDLE(ctypes.Structure):
    _fields_ = [("InitBusEnable", ctypes.c_bool)]


AXIS_INDEX = int(os.environ.get("AXIS_INDEX", "0"))
TARGET_SPEED = 3000000.0
TARGET_ACCEL = 3000000.0
TARGET_DECEL = 3000000.0
POLL_COUNT = 8
POLL_INTERVAL_SECONDS = 1.0
REQUIRED_ENCODER_DELTA = 1000
STEP_RETRY_COUNT = 5
STEP_RETRY_DELAY_SECONDS = 0.3


lib = ctypes.CDLL("/home/librobot.so.1.0.0")

lib.OpenRpmsg.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.OpenRpmsg.restype = ctypes.c_int32
lib.BusCmd_InitBus.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.BusCmd_InitBus.restype = ctypes.c_int32
lib.Weld_Direct_SetTargetVel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Weld_Direct_SetTargetVel.restype = ctypes.c_int32
lib.Weld_Direct_SetDirection.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]
lib.Weld_Direct_SetDirection.restype = ctypes.c_int32
lib.Weld_Direct_SetAType.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]
lib.Weld_Direct_SetAType.restype = ctypes.c_int32
lib.Weld_Direct_SetSingleTransper.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Weld_Direct_SetSingleTransper.restype = ctypes.c_int32
lib.Weld_Direct_SetAccel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Weld_Direct_SetAccel.restype = ctypes.c_int32
lib.Weld_Direct_SetDecel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Weld_Direct_SetDecel.restype = ctypes.c_int32
lib.Weld_Direct_SetAxisEnable.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]
lib.Weld_Direct_SetAxisEnable.restype = ctypes.c_int32
lib.Weld_Direct_GetAccel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_double)]
lib.Weld_Direct_GetAccel.restype = ctypes.c_int32
lib.Weld_Direct_GetDecel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_double)]
lib.Weld_Direct_GetDecel.restype = ctypes.c_int32
lib.Weld_Direct_GetEncoder.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetEncoder.restype = ctypes.c_int32
lib.Weld_Direct_GetAxisStatus.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAxisStatus.restype = ctypes.c_int32
lib.Weld_Direct_GetStatusCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetStatusCode.restype = ctypes.c_int32
lib.Weld_Direct_GetErrorCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetErrorCode.restype = ctypes.c_int32

HAS_GET_ACTUAL_VEL = hasattr(lib, "Weld_Direct_GetActualVel")
if HAS_GET_ACTUAL_VEL:
    lib.Weld_Direct_GetActualVel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetActualVel.restype = ctypes.c_int32

HAS_GET_COMMAND_TARGET_VEL = hasattr(lib, "Weld_Direct_GetCommandTargetVel")
if HAS_GET_COMMAND_TARGET_VEL:
    lib.Weld_Direct_GetCommandTargetVel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetCommandTargetVel.restype = ctypes.c_int32

HAS_GET_DRIVE_TARGET_VEL = hasattr(lib, "Weld_Direct_GetDriveTargetVel")
if HAS_GET_DRIVE_TARGET_VEL:
    lib.Weld_Direct_GetDriveTargetVel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetDriveTargetVel.restype = ctypes.c_int32

HAS_GET_DRIVE_MODE = hasattr(lib, "Weld_Direct_GetDriveMode")
if HAS_GET_DRIVE_MODE:
    lib.Weld_Direct_GetDriveMode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetDriveMode.restype = ctypes.c_int32

HAS_GET_DRIVE_DISPLAY = hasattr(lib, "Weld_Direct_GetDriveDisplay")
if HAS_GET_DRIVE_DISPLAY:
    lib.Weld_Direct_GetDriveDisplay.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetDriveDisplay.restype = ctypes.c_int32

HAS_GET_DRIVE_CONTROL_WORD = hasattr(lib, "Weld_Direct_GetDriveControlWord")
if HAS_GET_DRIVE_CONTROL_WORD:
    lib.Weld_Direct_GetDriveControlWord.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetDriveControlWord.restype = ctypes.c_int32

HAS_GET_DRIVE_STATUS_WORD = hasattr(lib, "Weld_Direct_GetDriveStatusWord")
if HAS_GET_DRIVE_STATUS_WORD:
    lib.Weld_Direct_GetDriveStatusWord.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
    lib.Weld_Direct_GetDriveStatusWord.restype = ctypes.c_int32


def log(message):
    print(message, flush=True)


def call_step(name, fn, *args):
    last_ret = 0
    for attempt in range(1, STEP_RETRY_COUNT + 1):
        ret = fn(*args)
        last_ret = ret
        if ret == 0:
            if attempt == 1:
                log(f"{name} {ret}")
            else:
                log(f"{name} {ret} retry={attempt}")
            return
        log(f"{name} {ret} retry={attempt}")
        if ret != 4 or attempt == STEP_RETRY_COUNT:
            break
        time.sleep(STEP_RETRY_DELAY_SECONDS)
    raise RuntimeError(f"{name} failed with error code {last_ret}")


def read_double(name, handle):
    value = ctypes.c_double()
    ret = getattr(lib, name)(ctypes.byref(handle), AXIS_INDEX, ctypes.byref(value))
    if ret != 0:
        raise RuntimeError(f"{name} failed with error code {ret}")
    return value.value


def read_int(name, handle):
    value = ctypes.c_int32()
    ret = getattr(lib, name)(ctypes.byref(handle), AXIS_INDEX, ctypes.byref(value))
    if ret != 0:
        raise RuntimeError(f"{name} failed with error code {ret}")
    return value.value


def read_optional_int(name, handle, available):
    if not available:
        return None
    return read_int(name, handle)


def main():
    if len(sys.argv) > 1:
        global AXIS_INDEX
        AXIS_INDEX = int(sys.argv[1])

    log(f"VERIFY axis={AXIS_INDEX}")
    handle = SYS_HANDLE()
    call_step("OpenRpmsg", lib.OpenRpmsg, ctypes.byref(handle))
    init_bus_ret = lib.BusCmd_InitBus(ctypes.byref(handle))
    log(f"BusCmd_InitBus {init_bus_ret}")
    if init_bus_ret not in (0, 4):
        raise RuntimeError(f"BusCmd_InitBus failed with error code {init_bus_ret}")
    call_step("SetDirection", lib.Weld_Direct_SetDirection, ctypes.byref(handle), AXIS_INDEX, 1)
    call_step("SetAType", lib.Weld_Direct_SetAType, ctypes.byref(handle), AXIS_INDEX, 3)
    call_step("SetSingleTransper", lib.Weld_Direct_SetSingleTransper, ctypes.byref(handle), AXIS_INDEX, 1.0)
    call_step("SetAccel", lib.Weld_Direct_SetAccel, ctypes.byref(handle), AXIS_INDEX, TARGET_ACCEL)
    call_step("SetDecel", lib.Weld_Direct_SetDecel, ctypes.byref(handle), AXIS_INDEX, TARGET_DECEL)
    call_step("SetAxisEnable", lib.Weld_Direct_SetAxisEnable, ctypes.byref(handle), AXIS_INDEX, 15)
    call_step("SetTargetVel", lib.Weld_Direct_SetTargetVel, ctypes.byref(handle), AXIS_INDEX, TARGET_SPEED)

    accel = read_double("Weld_Direct_GetAccel", handle)
    decel = read_double("Weld_Direct_GetDecel", handle)
    log(f"SUMMARY accel={accel:.1f} decel={decel:.1f}")

    encoders = []
    axis_statuses = []
    status_codes = []
    error_codes = []
    actual_vels = []
    command_targets = []
    drive_targets = []
    drive_modes = []
    drive_displays = []
    drive_ctrl_words = []
    drive_status_words = []

    for index in range(POLL_COUNT):
        encoder = read_int("Weld_Direct_GetEncoder", handle)
        actual_vel = read_optional_int("Weld_Direct_GetActualVel", handle, HAS_GET_ACTUAL_VEL)
        axis_status = read_int("Weld_Direct_GetAxisStatus", handle)
        status_code = read_int("Weld_Direct_GetStatusCode", handle)
        error_code = read_int("Weld_Direct_GetErrorCode", handle)
        command_target = read_optional_int(
            "Weld_Direct_GetCommandTargetVel",
            handle,
            HAS_GET_COMMAND_TARGET_VEL,
        )
        drive_target = read_optional_int(
            "Weld_Direct_GetDriveTargetVel",
            handle,
            HAS_GET_DRIVE_TARGET_VEL,
        )
        drive_mode = read_optional_int("Weld_Direct_GetDriveMode", handle, HAS_GET_DRIVE_MODE)
        drive_display = read_optional_int("Weld_Direct_GetDriveDisplay", handle, HAS_GET_DRIVE_DISPLAY)
        drive_ctrl_word = read_optional_int("Weld_Direct_GetDriveControlWord", handle, HAS_GET_DRIVE_CONTROL_WORD)
        drive_status_word = read_optional_int("Weld_Direct_GetDriveStatusWord", handle, HAS_GET_DRIVE_STATUS_WORD)
        encoders.append(encoder)
        actual_vels.append(0 if actual_vel is None else actual_vel)
        axis_statuses.append(axis_status)
        status_codes.append(status_code)
        error_codes.append(error_code)
        command_targets.append(0 if command_target is None else command_target)
        drive_targets.append(0 if drive_target is None else drive_target)
        drive_modes.append(0 if drive_mode is None else drive_mode)
        drive_displays.append(0 if drive_display is None else drive_display)
        drive_ctrl_words.append(0 if drive_ctrl_word is None else drive_ctrl_word)
        drive_status_words.append(0 if drive_status_word is None else drive_status_word)
        log(
            f"POLL {index + 1} encoder={encoder} actualVel={actual_vel if actual_vel is not None else 'NA'} "
            f"cmdTarget={command_target if command_target is not None else 'NA'} "
            f"driveTarget={drive_target if drive_target is not None else 'NA'} "
            f"driveMode={drive_mode if drive_mode is not None else 'NA'} "
            f"driveDisplay={drive_display if drive_display is not None else 'NA'} "
            f"driveCtrl={drive_ctrl_word if drive_ctrl_word is not None else 'NA'} "
            f"driveStatus={drive_status_word if drive_status_word is not None else 'NA'} "
            f"axisStatus={axis_status} statusCode={status_code} errorCode={error_code}"
        )
        time.sleep(POLL_INTERVAL_SECONDS)

    encoder_delta = abs(encoders[-1] - encoders[0]) if len(encoders) >= 2 else 0
    moved = encoder_delta >= REQUIRED_ENCODER_DELTA
    enabled = any(status == 1 for status in axis_statuses)
    accel_ok = abs(accel - TARGET_ACCEL) < 0.5
    decel_ok = abs(decel - TARGET_DECEL) < 0.5

    log(
        "RESULT "
        f"accel_ok={int(accel_ok)} decel_ok={int(decel_ok)} "
        f"enabled={int(enabled)} moved={int(moved)} encoder_delta={encoder_delta}"
    )
    log("ACTUAL_VELS " + ",".join(str(value) for value in actual_vels))
    log("CMD_TARGETS " + ",".join(str(value) for value in command_targets))
    log("DRIVE_TARGETS " + ",".join(str(value) for value in drive_targets))
    log("DRIVE_MODES " + ",".join(str(value) for value in drive_modes))
    log("DRIVE_DISPLAYS " + ",".join(str(value) for value in drive_displays))
    log("DRIVE_CTRL_WORDS " + ",".join(str(value) for value in drive_ctrl_words))
    log("DRIVE_STATUS_WORDS " + ",".join(str(value) for value in drive_status_words))
    log("STATUS_CODES " + ",".join(str(code) for code in status_codes))
    log("ERROR_CODES " + ",".join(str(code) for code in error_codes))

    if accel_ok and decel_ok and enabled and moved:
        log("VERIFY PASS")
        return 0

    log("VERIFY FAIL")
    return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        log(f"ERROR {exc}")
        sys.exit(1)
