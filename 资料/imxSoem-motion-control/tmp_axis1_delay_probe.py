import ctypes
import time


class SYS_HANDLE(ctypes.Structure):
    _fields_ = [("InitBusEnable", ctypes.c_bool)]


lib = ctypes.CDLL("/home/librobot.so.1.0.0")

bindings = [
    ("OpenRpmsg", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE)]),
    ("BusCmd_InitBus", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE)]),
    ("Weld_Direct_SetDirection", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]),
    ("Weld_Direct_SetAType", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]),
    ("Weld_Direct_SetSingleTransper", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]),
    ("Weld_Direct_SetAccel", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]),
    ("Weld_Direct_SetDecel", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]),
    ("Weld_Direct_SetAxisEnable", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_int32]),
    ("Weld_Direct_SetTargetVel", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.c_double]),
    ("Weld_Direct_GetErrorCode", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]),
    ("Weld_Direct_GetStatusCode", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]),
    ("Weld_Direct_GetEncoder", ctypes.c_int32, [ctypes.POINTER(SYS_HANDLE), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32)]),
]

for name, restype, argtypes in bindings:
    fn = getattr(lib, name)
    fn.restype = restype
    fn.argtypes = argtypes

h = SYS_HANDLE()
axis = 1

print("OpenRpmsg", lib.OpenRpmsg(ctypes.byref(h)), flush=True)
print("BusCmd_InitBus", lib.BusCmd_InitBus(ctypes.byref(h)), flush=True)
time.sleep(3.0)

steps = [
    ("SetDirection", lib.Weld_Direct_SetDirection, (ctypes.byref(h), axis, 1)),
    ("SetAType", lib.Weld_Direct_SetAType, (ctypes.byref(h), axis, 3)),
    ("SetSingleTransper", lib.Weld_Direct_SetSingleTransper, (ctypes.byref(h), axis, 1.0)),
    ("SetAccel", lib.Weld_Direct_SetAccel, (ctypes.byref(h), axis, 3000000.0)),
    ("SetDecel", lib.Weld_Direct_SetDecel, (ctypes.byref(h), axis, 3000000.0)),
    ("SetAxisEnable", lib.Weld_Direct_SetAxisEnable, (ctypes.byref(h), axis, 15)),
    ("SetTargetVel", lib.Weld_Direct_SetTargetVel, (ctypes.byref(h), axis, 3000000.0)),
]

for name, fn, args in steps:
    print(name, fn(*args), flush=True)
    time.sleep(0.5)

for i in range(8):
    encoder = ctypes.c_int32()
    status = ctypes.c_int32()
    error = ctypes.c_int32()
    lib.Weld_Direct_GetEncoder(ctypes.byref(h), axis, ctypes.byref(encoder))
    lib.Weld_Direct_GetStatusCode(ctypes.byref(h), axis, ctypes.byref(status))
    lib.Weld_Direct_GetErrorCode(ctypes.byref(h), axis, ctypes.byref(error))
    print(
        f"poll {i + 1} encoder={encoder.value} status={status.value} error={error.value}",
        flush=True,
    )
    time.sleep(1.0)
