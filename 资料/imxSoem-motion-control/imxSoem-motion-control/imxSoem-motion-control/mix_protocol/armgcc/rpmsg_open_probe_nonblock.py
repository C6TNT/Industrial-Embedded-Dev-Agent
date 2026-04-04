import os
for path in ('/dev/rpmsg0', '/dev/rpmsg1'):
    try:
        fd = os.open(path, os.O_RDWR | os.O_NONBLOCK)
    except OSError as exc:
        print(f'{path} ERR errno={exc.errno} msg={exc}')
    else:
        print(f'{path} OK fd={fd}')
        os.close(fd)