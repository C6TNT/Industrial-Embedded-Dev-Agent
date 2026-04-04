import ctypes
import time

class SYS_HANDLE(ctypes.Structure):
    _fields_ = [('InitBusEnable', ctypes.c_bool)]

lib = ctypes.CDLL('/home/librobot.so.1.0.0')
AXIS = 0
TARGET_SPEED = 3000000.0
TARGET_ACCEL = 3000000.0
TARGET_DECEL = 3000000.0

lib.OpenRpmsg.argtypes = [ctypes.POINTER(SYS_HANDLE)]
lib.OpenRpmsg.restype = ctypes.c_int32
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

h = SYS_HANDLE()
print('OpenRpmsg', lib.OpenRpmsg(ctypes.byref(h)), flush=True)
print('SetTargetVel', lib.Weld_Direct_SetTargetVel(ctypes.byref(h), AXIS, TARGET_SPEED), flush=True)
print('SetDirection', lib.Weld_Direct_SetDirection(ctypes.byref(h), AXIS, 1), flush=True)
print('SetAType', lib.Weld_Direct_SetAType(ctypes.byref(h), AXIS, 3), flush=True)
print('SetSingleTransper', lib.Weld_Direct_SetSingleTransper(ctypes.byref(h), AXIS, 1.0), flush=True)
print('SetAccel', lib.Weld_Direct_SetAccel(ctypes.byref(h), AXIS, TARGET_ACCEL), flush=True)
print('SetDecel', lib.Weld_Direct_SetDecel(ctypes.byref(h), AXIS, TARGET_DECEL), flush=True)
print('SetAxisEnable', lib.Weld_Direct_SetAxisEnable(ctypes.byref(h), AXIS, 15), flush=True)

for i in range(8):
    acc = ctypes.c_double()
    dec = ctypes.c_double()
    enc = ctypes.c_int32()
    st = ctypes.c_int32()
    code = ctypes.c_int32()
    r1 = lib.Weld_Direct_GetAccel(ctypes.byref(h), AXIS, ctypes.byref(acc))
    r2 = lib.Weld_Direct_GetDecel(ctypes.byref(h), AXIS, ctypes.byref(dec))
    r3 = lib.Weld_Direct_GetEncoder(ctypes.byref(h), AXIS, ctypes.byref(enc))
    r4 = lib.Weld_Direct_GetAxisStatus(ctypes.byref(h), AXIS, ctypes.byref(st))
    r5 = lib.Weld_Direct_GetStatusCode(ctypes.byref(h), AXIS, ctypes.byref(code))
    print(f'poll={i+1} accel_ret={r1} accel={acc.value} decel_ret={r2} decel={dec.value} enc_ret={r3} enc={enc.value} axisStatus={st.value} statusCode={code.value}', flush=True)
    time.sleep(1)
