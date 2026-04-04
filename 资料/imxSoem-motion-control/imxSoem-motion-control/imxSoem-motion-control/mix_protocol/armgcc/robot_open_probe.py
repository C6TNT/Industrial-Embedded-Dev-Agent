import os
import signal
import time


def try_open(flags, label, seconds=None):
    if seconds is not None:
        signal.alarm(seconds)
    start = time.time()
    try:
        fd = os.open("/dev/rpmsg0", flags)
        print(f"{label}: open-ok fd={fd} elapsed={time.time() - start:.3f}", flush=True)
        os.close(fd)
    except BaseException as exc:
        errno_value = getattr(exc, "errno", None)
        print(
            f"{label}: open-fail type={type(exc).__name__} errno={errno_value} elapsed={time.time() - start:.3f}",
            flush=True,
        )
    finally:
        signal.alarm(0)


if __name__ == "__main__":
    try_open(os.O_RDWR | os.O_NONBLOCK, "nonblock")
    try_open(os.O_RDWR, "blocking", seconds=5)
