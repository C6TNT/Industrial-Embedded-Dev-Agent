import ctypes


class SYS_HANDLE(ctypes.Structure):
    _fields_ = [("InitBusEnable", ctypes.c_bool)]


lib = ctypes.CDLL("./librobot.so.1.0.0")

lib.OpenRpmsg.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.OpenRpmsg.restype = ctypes.c_int32

lib.BusCmd_InitBus.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.BusCmd_InitBus.restype = ctypes.c_int32

lib.Direct_SetDirection.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]
lib.Direct_SetDirection.restype = ctypes.c_int32

lib.Direct_SetAType.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]
lib.Direct_SetAType.restype = ctypes.c_int32

lib.Direct_SetSingleTransper.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Direct_SetSingleTransper.restype = ctypes.c_int32

lib.Direct_SetAccel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Direct_SetAccel.restype = ctypes.c_int32

lib.Direct_SetDecel.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]
lib.Direct_SetDecel.restype = ctypes.c_int32

lib.Direct_SetAxisEnable.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]
lib.Direct_SetAxisEnable.restype = ctypes.c_int32

lib.Direct_SetSingleParam.argtypes = [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double, ctypes.c_double]
lib.Direct_SetSingleParam.restype = ctypes.c_int32


def check_ret(name, ret):
    if ret != 0:
        raise RuntimeError(f"{name} failed with error code {ret}")


def main():
    handle = SYS_HANDLE()
    axis = 0

    check_ret("OpenRpmsg", lib.OpenRpmsg(ctypes.byref(handle)))
    check_ret("BusCmd_InitBus", lib.BusCmd_InitBus(ctypes.byref(handle)))

    check_ret("Weld_Direct_SetDirection", lib.Direct_SetDirection(ctypes.byref(handle), axis, 1))
    check_ret("Weld_Direct_SetAType", lib.Direct_SetAType(ctypes.byref(handle), axis, 66))
    check_ret("Weld_Direct_SetSingleTransper", lib.Direct_SetSingleTransper(ctypes.byref(handle), axis, 1.0))
    check_ret("Weld_Direct_SetAccel", lib.Direct_SetAccel(ctypes.byref(handle), axis, 1000000.0))
    check_ret("Weld_Direct_SetDecel", lib.Direct_SetDecel(ctypes.byref(handle), axis, 1000000.0))
    check_ret("Weld_Direct_SetAxisEnable", lib.Direct_SetAxisEnable(ctypes.byref(handle), axis, 1))
    check_ret("Weld_Direct_SetSingleParam", lib.Direct_SetSingleParam(ctypes.byref(handle), axis, 10000.0, 10000.0))

    print("Axis initialized successfully.")


if __name__ == "__main__":
    main()

