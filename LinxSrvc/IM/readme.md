<h3 align = "center">KaiSocket Usage</h3>

* Initialize with sentence `KaiSocket::Initialize(PORT);` is first of all. Call `registerCallback` to register a callback with `KAISOCKHOOK` format, the function can used to `recv` and `send` message, buffer size and data was needed to set. Also, add more callbacks by calling `appendCallback`, therefore, just need to be attention to is how dealing with the data rather than the pesky socket transmission. At last, call `start()` to start a new `KaiSocket` server process.

* Writing a client, just register callbacks and using `connect()` to connect a server whose IP and PORT sets when calling like `Initialize(IP, PORT)` above.

* e.g. [*here*](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/KaiTest.cpp) is an usage case.
