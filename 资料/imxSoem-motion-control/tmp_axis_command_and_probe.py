import ctypes
import time


class SYS_HANDLE(ctypes.Structure):
    _fields_ = [("InitBusEnable", ctypes.c_bool)]


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
lib.Weld_Direct_GetErrorCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetErrorCode.restype = ctypes.c_int32
lib.Weld_Direct_GetStatusCode.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetStatusCode.restype = ctypes.c_int32
lib.Weld_Direct_GetAxisStatus.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAxisStatus.restype = ctypes.c_int32
lib.Weld_Direct_GetEncoder.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetEncoder.restype = ctypes.c_int32
lib.Weld_Direct_GetAType.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAType.restype = ctypes.c_int32
lib.Weld_Direct_GetAxisEnable.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]
lib.Weld_Direct_GetAxisEnable.restype = ctypes.c_int32


def log(msg):
    print(msg, flush=True)


def call(name, fn, *args):
    ret = fn(*args)
    log(f"{name} ret={ret}")
    return ret


def read_int(name, fn, handle, axis):
    value = ctypes.c_int32()
    ret = fn(ctypes.byref(handle), axis, ctypes.byref(value))
    log(f"{name} axis={axis} ret={ret} value={value.value}")
    return ret, value.value


def main():
    handle = SYS_HANDLE()
    call("OpenRpmsg", lib.OpenRpmsg, ctypes.byref(handle))

    for axis in (0, 1):
        call("SetDirection", lib.Weld_Direct_SetDirection, ctypes.byref(handle), axis, 1)
        call("SetAType", lib.Weld_Direct_SetAType, ctypes.byref(handle), axis, 3)
        call("SetSingleTransper", lib.Weld_Direct_SetSingleTransper, ctypes.byref(handle), axis, 1.0)
        call("SetAccel", lib.Weld_Direct_SetAccel, ctypes.byref(handle), axis, 3000000.0)
        call("SetDecel", lib.Weld_Direct_SetDecel, ctypes.byref(handle), axis, 3000000.0)
        call("SetAxisEnable", lib.Weld_Direct_SetAxisEnable, ctypes.byref(handle), axis, 15)
        call("SetTargetVel", lib.Weld_Direct_SetTargetVel, ctypes.byref(handle), axis, 3000000.0)

    for poll in range(10):
        log(f"POLL {poll + 1}")
        for axis in (0, 1):
            read_int("GetAType", lib.Weld_Direct_GetAType, handle, axis)
            read_int("GetAxisEnable", lib.Weld_Direct_GetAxisEnable, handle, axis)
            read_int("GetErrorCode", lib.Weld_Direct_GetErrorCode, handle, axis)
            read_int("GetStatusCode", lib.Weld_Direct_GetStatusCode, handle, axis)
            read_int("GetAxisStatus", lib.Weld_Direct_GetAxisStatus, handle, axis)
            read_int("GetEncoder", lib.Weld_Direct_GetEncoder, handle, axis)
        time.sleep(1.0)


if __name__ == "__main__":
    main()
