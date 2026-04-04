import ctypes, time
class SYS_HANDLE(ctypes.Structure):
    _fields_=[('InitBusEnable', ctypes.c_bool)]
lib=ctypes.CDLL('/home/librobot.so.1.0.0')
for name,args,restype in [
('OpenRpmsg',[ctypes.POINTER(SYS_HANDLE)],ctypes.c_int32),
('Weld_Direct_SetTargetVel',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_double],ctypes.c_int32),
('Weld_Direct_SetDirection',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_int32],ctypes.c_int32),
('Weld_Direct_SetAType',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_int32],ctypes.c_int32),
('Weld_Direct_SetSingleTransper',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_double],ctypes.c_int32),
('Weld_Direct_SetAccel',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_double],ctypes.c_int32),
('Weld_Direct_SetDecel',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_double],ctypes.c_int32),
('Weld_Direct_SetAxisEnable',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.c_int32],ctypes.c_int32),
('Weld_Direct_GetEncoder',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.POINTER(ctypes.c_int32)],ctypes.c_int32),
('Weld_Direct_GetAxisStatus',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.POINTER(ctypes.c_int32)],ctypes.c_int32),
('Weld_Direct_GetStatusCode',[ctypes.POINTER(SYS_HANDLE),ctypes.c_int32,ctypes.POINTER(ctypes.c_int32)],ctypes.c_int32),
]:
    getattr(lib,name).argtypes=args
    getattr(lib,name).restype=restype
h=SYS_HANDLE()
axis=0
steps=[
('OpenRpmsg', lambda: lib.OpenRpmsg(ctypes.byref(h))),
('SetTargetVel', lambda: lib.Weld_Direct_SetTargetVel(ctypes.byref(h),axis,3000000.0)),
('SetDirection', lambda: lib.Weld_Direct_SetDirection(ctypes.byref(h),axis,1)),
('SetAType', lambda: lib.Weld_Direct_SetAType(ctypes.byref(h),axis,3)),
('SetSingleTransper', lambda: lib.Weld_Direct_SetSingleTransper(ctypes.byref(h),axis,1.0)),
('SetAccel', lambda: lib.Weld_Direct_SetAccel(ctypes.byref(h),axis,3000000.0)),
('SetDecel', lambda: lib.Weld_Direct_SetDecel(ctypes.byref(h),axis,3000000.0)),
('SetAxisEnable', lambda: lib.Weld_Direct_SetAxisEnable(ctypes.byref(h),axis,15)),
]
for n,f in steps:
    r=f(); print(n,r, flush=True)
enc=[]
for i in range(10):
    e=ctypes.c_int32(); s=ctypes.c_int32(); c=ctypes.c_int32()
    re=lib.Weld_Direct_GetEncoder(ctypes.byref(h),axis,ctypes.byref(e))
    rs=lib.Weld_Direct_GetAxisStatus(ctypes.byref(h),axis,ctypes.byref(s))
    rc=lib.Weld_Direct_GetStatusCode(ctypes.byref(h),axis,ctypes.byref(c))
    print('POLL',i+1,re,e.value,rs,s.value,rc,c.value, flush=True)
    enc.append(e.value)
    time.sleep(1)
print('DELTA', abs(enc[-1]-enc[0]), flush=True)
