import os
import errno
for path in ('/dev/rpmsg0', '/dev/rpmsg1'):
    for flags, name in ((os.O_RDWR, 'RDWR'), (os.O_RDWR | os.O_NONBLOCK, 'RDWR_NONBLOCK')):
        try:
            fd = os.open(path, flags)
        except OSError as exc:
            print(f'{path} {name} ERR errno={exc.errno} msg={exc}')
        else:
            print(f'{path} {name} OK fd={fd}')
            os.close(fd)