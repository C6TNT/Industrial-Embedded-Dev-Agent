import os
import time


for path in ("/dev/rpmsg0", "/dev/rpmsg1", "/dev/rpmsg2", "/dev/rpmsg3"):
    start = time.time()
    try:
        fd = os.open(path, os.O_RDWR | os.O_NONBLOCK)
        print(f"{path}: open-ok fd={fd} elapsed={time.time() - start:.3f}", flush=True)
        os.close(fd)
    except OSError as exc:
        print(f"{path}: open-fail errno={exc.errno} elapsed={time.time() - start:.3f}", flush=True)
