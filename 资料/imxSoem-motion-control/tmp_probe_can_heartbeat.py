import ctypes
import os
import sys
import time


class SYS_HANDLE(ctypes.Structure):
    _fields_ = [("InitBusEnable", ctypes.c_bool)]


lib = ctypes.CDLL("/home/librobot.so.1.0.0")

lib.OpenRpmsg.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.OpenRpmsg.restype = ctypes.c_int32
lib.BusCmd_InitBus.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.BusCmd_InitBus.restype = ctypes.c_int32
lib.Weld_Direct_GetErrorCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetErrorCode.restype = ctypes.c_int32
lib.Weld_Direct_GetEncoder.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetEncoder.restype = ctypes.c_int32
lib.Weld_Direct_GetAxisStatus.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAxisStatus.restype = ctypes.c_int32
lib.Weld_Direct_GetStatusCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetStatusCode.restype = ctypes.c_int32


POLL_COUNT = int(os.environ.get("POLL_COUNT", "8"))
POLL_INTERVAL_SECONDS = float(os.environ.get("POLL_INTERVAL_SECONDS", "1.0"))
AXES = (0, 1)


def log(message):
    print(message, flush=True)


def call_step(name, fn, *args):
    ret = fn(*args)
    log(f"{name} {ret}")
    return ret


def read_int(name, fn, handle, axis):
    value = ctypes.c_int32()
    ret = fn(ctypes.byref(handle), axis, ctypes.byref(value))
    return ret, value.value


def read_axis_snapshot(handle, axis):
    error_ret, error_code = read_int("GetErrorCode", lib.Weld_Direct_GetErrorCode, handle, axis)
    status_ret, status_code = read_int("GetStatusCode", lib.Weld_Direct_GetStatusCode, handle, axis)
    axis_ret, axis_status = read_int("GetAxisStatus", lib.Weld_Direct_GetAxisStatus, handle, axis)
    encoder_ret, encoder = read_int("GetEncoder", lib.Weld_Direct_GetEncoder, handle, axis)
    log(
        f"AXIS {axis} "
        f"errorRet={error_ret} errorCode={error_code} "
        f"statusRet={status_ret} statusCode={status_code} "
        f"axisRet={axis_ret} axisStatus={axis_status} "
        f"encoderRet={encoder_ret} encoder={encoder}"
    )


def main():
    handle = SYS_HANDLE()
    call_step("OpenRpmsg", lib.OpenRpmsg, ctypes.byref(handle))

    if "--init-bus" in sys.argv:
        call_step("BusCmd_InitBus", lib.BusCmd_InitBus, ctypes.byref(handle))

    for index in range(POLL_COUNT):
        log(f"POLL {index + 1}")
        for axis in AXES:
            read_axis_snapshot(handle, axis)
        time.sleep(POLL_INTERVAL_SECONDS)

    return 0


if __name__ == "__main__":
    sys.exit(main())
