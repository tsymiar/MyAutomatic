# -*- coding: utf-8 -*-
# filename: main.py
import sys, os
import web
from handle import Handle

urls = (
    '/wx', 'Handle',
)

def main():
    app = web.application(urls, globals())
    app.run()

if __name__ == '__main__':
    # do the UNIX double-fork magic
    try:
        pid = os.fork()
        if pid > 0:
            # exit first parent
            sys.exit(0)
    except OSError, e:
        print >>sys.stderr, "fork #1 failed: %d (%s)" % (e.errno, e.strerror)
        sys.exit(1)
    os.chdir("/")
    os.setsid()
    os.umask(0)
    try:
        pid = os.fork()
        if pid > 0:
            print "Daemon PID %d" % pid
            sys.exit(0)
    except OSError, e:
        print >>sys.stderr, "fork #2 failed: %d (%s)" % (e.errno, e.strerror)
        sys.exit(1)
    main()
