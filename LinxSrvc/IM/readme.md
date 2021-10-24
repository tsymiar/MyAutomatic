<h3 align = "center">KaiSocket Usage</h3>

* Initialize with sentence `KaiSocket::Initialize(PORT);`. Then call `registerCallback` to register a callback in `KAISOCKHOOK` type, the function can used to `recv` or `send` message, buffer and data should be needed. Also, add other callbacks by calling `appendCallback`, therefore, just need to be attention to is how dealing the data rather than the pesky socket transmission. At last, call `start()` to start a new `KaiSocket` server process.

* Writing a client, just register callbacks and using `connect()` to connect a server whose IP and PORT sets when calling like `Initialize(IP, PORT)` above.

* e.g. [here](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/KaiTest.cpp) is an usage case.
