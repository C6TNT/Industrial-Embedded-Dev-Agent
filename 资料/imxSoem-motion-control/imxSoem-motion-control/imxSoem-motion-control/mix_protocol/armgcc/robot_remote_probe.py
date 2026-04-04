import ctypes
import os
import sys
import time


def log(message):
    print(message, flush=True)


def main():
    log(f"cwd={os.getcwd()}")
    log(f"python={sys.version.split()[0]}")
    log("loading librobot...")
    lib = ctypes.CDLL("/home/librobot.so.1.0.0")
    log("librobot loaded")

    class SYS_HANDLE(ctypes.Structure):
        _fields_ = [("InitBusEnable", ctypes.c_bool)]

    lib.OpenRpmsg.argtypes = [ctypes.POINTER(SYS_HANDLE)]
    lib.OpenRpmsg.restype = ctypes.c_int32

    handle = SYS_HANDLE()
    start = time.time()
    log("calling OpenRpmsg...")
    ret = lib.OpenRpmsg(ctypes.byref(handle))
    log(f"OpenRpmsg ret={ret} elapsed={time.time() - start:.3f}s")


if __name__ == "__main__":
    main()
