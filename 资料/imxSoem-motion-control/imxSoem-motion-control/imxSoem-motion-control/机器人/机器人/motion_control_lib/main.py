import ctypes
import os
import sys
import time


class SYS_HANDLE(ctypes.Structure):
    _fields_ = [("InitBusEnable", ctypes.c_bool)]


def log(message):
    print(f"[robot-main] {message}", flush=True)


lib = ctypes.CDLL("./librobot.so.1.0.0")

lib.OpenRpmsg.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.OpenRpmsg.restype = ctypes.c_int32
lib.BusCmd_InitBus.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.BusCmd_InitBus.restype = ctypes.c_int32

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

lib.Weld_Direct_SetTargetVel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Weld_Direct_SetTargetVel.restype = ctypes.c_int32

lib.Weld_Direct_GetAType.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAType.restype = ctypes.c_int32

lib.Weld_Direct_GetEncoder.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetEncoder.restype = ctypes.c_int32

lib.Weld_Direct_GetAxisStatus.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAxisStatus.restype = ctypes.c_int32

lib.Weld_Direct_GetStatusCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetStatusCode.restype = ctypes.c_int32

TARGET_ACCEL = 3000000.0
TARGET_DECEL = 3000000.0
TARGET_SPEED = 3000000.0
POLL_SECONDS = 30
AXIS_INDEX = 0
STEP_RETRY_COUNT = 5
STEP_RETRY_DELAY_SECONDS = 0.3


def call_step(name, fn, *args):
    last_ret = 0
    for attempt in range(1, STEP_RETRY_COUNT + 1):
        started = time.time()
        ret = fn(*args)
        elapsed = time.time() - started
        log(f"{name}: attempt={attempt} ret={ret} elapsed={elapsed:.3f}s")
        last_ret = ret
        if ret == 0:
            return ret
        if ret != 4 or attempt == STEP_RETRY_COUNT:
            break
        time.sleep(STEP_RETRY_DELAY_SECONDS)
    raise RuntimeError(f"{name} failed with error code {last_ret}")


def read_int_value(name, fn, handle, axis):
    value = ctypes.c_int32()
    ret = fn(ctypes.byref(handle), axis, ctypes.byref(value))
    log(f"{name}: ret={ret} value={value.value}")
    return ret, value.value


def main():
    handle = SYS_HANDLE()

    log(f"cwd={os.getcwd()}")
    log(f"python={sys.version.split()[0]}")
    log(f"axis={AXIS_INDEX}")

    # 1. 建立 A53 与 M7 的 RPMsg 通道，并显式启动总线相关任务。
    call_step("OpenRpmsg", lib.OpenRpmsg, ctypes.byref(handle))
    init_bus_ret = lib.BusCmd_InitBus(ctypes.byref(handle))
    log(f"BusCmd_InitBus: ret={init_bus_ret}")
    if init_bus_ret not in (0, 4):
        raise RuntimeError(f"BusCmd_InitBus failed with error code {init_bus_ret}")

    # 2. 先配置模式、方向、加减速和使能，最后再下发目标速度。
    call_step("Weld_Direct_SetDirection", lib.Weld_Direct_SetDirection, ctypes.byref(handle), AXIS_INDEX, 1)
    call_step("Weld_Direct_SetAType", lib.Weld_Direct_SetAType, ctypes.byref(handle), AXIS_INDEX, 3)
    call_step("Weld_Direct_SetSingleTransper", lib.Weld_Direct_SetSingleTransper, ctypes.byref(handle), AXIS_INDEX, 1.0)
    call_step("Weld_Direct_SetAccel", lib.Weld_Direct_SetAccel, ctypes.byref(handle), AXIS_INDEX, TARGET_ACCEL)
    call_step("Weld_Direct_SetDecel", lib.Weld_Direct_SetDecel, ctypes.byref(handle), AXIS_INDEX, TARGET_DECEL)
    call_step("Weld_Direct_SetAxisEnable", lib.Weld_Direct_SetAxisEnable, ctypes.byref(handle), AXIS_INDEX, 15)
    call_step("Weld_Direct_SetTargetVel", lib.Weld_Direct_SetTargetVel, ctypes.byref(handle), AXIS_INDEX, TARGET_SPEED)

    # 3. 运行期轮询关键反馈，确认模式、使能和编码器都在变化。
    log(f"Axis initialized successfully. continuous velocity mode for {POLL_SECONDS}s")
    for tick in range(POLL_SECONDS):
        time.sleep(1)
        log(f"poll={tick + 1}")
        read_int_value("Weld_Direct_GetAType", lib.Weld_Direct_GetAType, handle, AXIS_INDEX)
        read_int_value("Weld_Direct_GetAxisStatus", lib.Weld_Direct_GetAxisStatus, handle, AXIS_INDEX)
        read_int_value("Weld_Direct_GetStatusCode", lib.Weld_Direct_GetStatusCode, handle, AXIS_INDEX)
        read_int_value("Weld_Direct_GetEncoder", lib.Weld_Direct_GetEncoder, handle, AXIS_INDEX)

    # 联调阶段保留速度命令，便于继续观察稳态反馈。
    log("Velocity command remains active. No stop command sent.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        log(f"ERROR: {exc}")
        raise
