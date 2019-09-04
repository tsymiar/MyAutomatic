# -*- coding: utf-8 -*-
# filename: log.py
import logging

class Log:
    def __init__(self, xapp, logname="wsgi"):
        class O:
            def __init__(self, xapp, logname="wsgi"):
                self.logger = logging.getLogger(logname)
            def write(self, s):
                if s[-1] == '\n':
                    s = s[:-1]
                if s == "":
                    return
                self.logger.debug(s)
        self.app = xapp
        self.f = O(logname)
    def __call__(self, environ, start_response):
        environ['wsgi.errors'] = self.f
        return self.app(environ, start_response)
