<h1 align="center">MyAutomatic</h1>

[![CMake on multiple platforms](https://github.com/tsymiar/MyAutomatic/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=auto-dev)](https://github.com/tsymiar/MyAutomatic/actions/workflows/cmake-multi-platform.yml)
[![Build Status](https://tsymiar.visualstudio.com/MyAutomatic/_apis/build/status%2Ftsymiar.MyAutomatic?branchName=auto-dev)](https://tsymiar.visualstudio.com/MyAutomatic/_build/latest?definitionId=70&branchName=auto-dev)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://app.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

> A hands-on playground for **systems code**: instant messaging, a `SOAP` service, an `HTTP`/`WebSocket` server, Linux hardware and kernel drivers, `RDMA`, plus `Qt` / `C#` / `Python` / `Swift` side projects — all driven by one script.

```sh
git clone https://github.com/tsymiar/MyAutomatic.git
```

##### _top-level ⇣⇣⇣_

| Directory | Stack | What lives there |
| :-- | :-- | :-- |
| [LinxSrvc](#linxsrvc) | C / C++ / Python / Swift | the main project — services, drivers and tools (see the categories below) |
| [QtGames](#qtgames) | `Qt` + `SDL` + `OpenGL` | rendering / input test-cases |
| [WinNTKline](#winntkline) | `C#` / `WPF` / `MFC` / `C++` (**Windows**) | K-line desktop apps and their utility libs |
| Market | prebuilt `.exe` | demo binaries kept as built |
| toolset | Shell / Python | standalone helpers — `similary.py` (comparison backend for the VSCode extension), `matkline.py` (draws K-line figures), plus cache / git / gdb scripts |

### Build &amp; test

| Goal | Command |
| :-- | :-- |
| Build everything | `./build.sh all -j` |
| Unit tests + `lcov` coverage | `./build.sh test` |
| Remove caches and artifacts | `./build.sh clean` |
| Build a single CMake sub-project | `./build.sh <dir>` — e.g. `./build.sh lookup` |

Third-party sources sit in `LinxSrvc/3rd/` as submodules (`googletest`, `rdma-core`, `rxe-dev`, `json`, `lcov`); `./build.sh test` fetches the ones it needs.

---

LinxSrvc
--------

`./build.sh all` runs two independent chains — the top-level `Makefile` chain and the top-level `CMakeLists.txt` chain — so artifacts end up in one of two directories:

| Output | Artifacts |
| :-- | :-- |
| `bin/` (Make chain) | `IM.exe`, `client.exe`, `kaics.exe` (`kaics.cfg`), `gSOAPverify` (`myweb.wsdl`), `pthdtest.exe`, `VideoCapture`, `imgfilesnap.exe`, `chstest`, `chigpio`, `mes909`, `pipefifo`, `dirtyhack`, `test_phm2f.exe`, `rdma_server.exe`, `rdma_client.exe` |
| `gen/` (CMake chain) | `lookup`, `genSeek`, `seek_test` (`time.cfg`, `libtimeUtil.so`), `diffs`, `gn1`, `e.g`, `webevs_serve`, `mocking_main`, `mocking_client`, `dpsk_chat` (`params.txt`), `video_render`, `sudoku-*.whl`, `similarity-*.vsix` |

### Messaging &amp; networking

* **IM.exe | client.exe**

    [_`IM.exe`_](https://raw.githubusercontent.com/tsymiar/MyAutomatic/auto-dev/LinxSrvc/IM/IM.cc) is an `instant-messaging` chat room demo: register, login, send commands, and exchange _a small number_ of messages. It forks a child per connection, listens on `argv[1]` (default `8877`), and `-m` / `-x` print / reset the shared-memory online table.

    _client.exe_ is the chat-room peer with its own menus. Its sources live outside this folder (`WinNTKline/KlineUtil/IM/client.cc` and `WinNTKline/IMclient/imclient.cpp`), so it only builds when that sibling project is checked out too.

* **kaics.exe**

    A _sub-pub_ message queue (_`MQ`_) that can penetrate the intranet — the `KaiTest` CLI linked against `KaiSocket` (`-S` / `-C` / `-BK` / `-SS [topic]` / `-PB [topic] [msg]` / `-TF [topic] [file]`, endpoint from `kaics.cfg`, default `127.0.0.1:9999`). More in [its readme](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/readme.md).

* **webevs_serve**

    HTTP + WebSocket server built on `libevent` (HTTP) and `libwebsockets` (WS). The WS port runs alongside the HTTP one: launch with `./webevs_serve <http_port> [ws_port]`, or set `WEBEV_WS_PORT`.

* **gSOAPverify**

    A `SOAP` server that verifies logins against the contract in `myweb.wsdl`.

* **mocking_main | mocking_client**

    [_`UDP`_/_`TCP`_/multicast] throughput tool pair in _C++11_. Both come from the same `socket.cpp`, split by the `CLIENT` macro — useful as a baseline when sizing the `Mac/Transfer` and KaiSocket paths.

* **rdma_server.exe | rdma_client.exe**

    _`RDMA`_ sample pair for `TCP/IP` / `Rocket Direct` applications, on top of _librdmacm.so_.

### Search, data generation &amp; code analysis

* **lookup | genSeek | seek_test**

    _lookup_ searches recorded streams with `kmp` / `manacher` (`lookup <file> [pattern]`, default = longest palindrome). _genSeek_ emits synthetic frames to test with, and _seek_test_ exercises the time-index engine (V1 = `seek/`, V2 = `time/`). `time.cfg` and `libtimeUtil.so` land in _gen_ as well — note that index/query go through SQLite and silently no-op when built without `USE_SQLITE3`.

* **gn1**

    A _cross-platform_, _big/small endian_, _increasing/decreasing_ binary number generator.

* **diffs**

    Compares differences between two same-named files or directories (CMake target renamed from `analyzing` to **`diffs`**). Similarity is computed after stripping comments and whitespace.

    <img src="assets/diff.png" title="diffs" width="50%" height="auto" />

### Concurrency

* **pthdtest.exe**

    A thread pool based on `pthread` (`LinxSrvc/thread/pthdpool`).

### Hardware &amp; drivers (`LinxSrvc/hdware/`)

| Tool | What it exercises |
| :-- | :-- |
| chstest | sample client for the `chsdev` character device |
| chigpio | `GPIO` lines |
| mes909 | `ME909S-821`, a Huawei `LTE 4G` module |
| pipefifo | `pipe` / `fifo` samples |
| dirtyhack | the classic DirtyCOW write-up in code form |
| test_phm2f | tiny test showing `phy_mem.ko` usage |
| VideoCapture | video capture with **v4l2** (Linux only) |
| imgfilesnap | snapshot grabber (Linux only) |

### Media &amp; AI

* **video_render**

    A video decode demo using `ffmpeg` / `multimedia` (Jetson Orin Nano).

* **dpsk_chat**

    A mini chat tool built on the _`DeepSeek`_ API. Export `DPSK_API_KEY` before launching — otherwise it prints a notice and exits (`dpsk/CurlReqs.cpp:254`). ⚠️ Key-value pairs such as model or stream live in `params.txt`, which must sit in the working directory (a copy lands in _gen_).

    <img src="assets/dpsk.jpg" title="DeepSeek" onclick="javascript:location.href='https://www.deepseek.com'" width="70%" height="auto" />

### Projects outside the two build chains

| Project | What it is | How to build / run |
| :-- | :-- | :-- |
| [Mac/Transfer](LinxSrvc/Mac/Transfer/README.md) | macOS `SwiftUI` app over a C++17 `FTF` transfer engine (64-byte header, port `8800`) | needs macOS + Swift 5.9 |
| cyber | `LoRA` fine-tuning pipeline (`PEFT`, 4-bit capable) over chat exports | `pip install -r LinxSrvc/cyber/requirements.txt`, then its scripts — see its readme |
| trade | single-file quantitative strategy (`dual_ma_strategy.py` + `strategy.yaml`) with Backtrader and an LSTM branch | `python dual_ma_strategy.py` after installing the deps |
| vs-extension | VSCode "Files Comparison" extension; Python backend symlinked from `toolset/similary.py` | `make.sh` inside the folder, packages `similarity-*.vsix` into _gen_ |
| pyex | a `sudoku` CPython C extension | `python -m build`, wheel into _gen_ |
| test | `GoogleTest` + `lcov` harness, depends on `3rd/googletest` and `webevs/Utils.cpp` | `./build.sh test` (or `./test.sh` inside the folder) |
| shell | ops helpers: `x64mk/` builds an amd64 APK under QEMU, `expect/` automates logins (contains plaintext passwords — keep private) | — |

QtGames
-------

* [_`It`_](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/QtGames) is a test-case using _`Qt`_, _`SDL`_ and _`OpenGL`_. Use _mkallcase.sh_ to build it (or `./build.sh QtGames/` from the repository root).

WinNTKline
----------

##### [Microsoft .NET Framework 3.5](https://aka.ms/msbuild/developerpacks) is needed to compile WinNTKline

| Project | What it is |
| :-- | :-- |
| CvMlwk | _`OpenCV`_ and some _`Machine Learning`_ learning cases |
| KlineUtil | utils for `cef-browser`, security libs, drawing _K-line_ with GL, simulating `CTP`, etc. |
| IMclient | the Windows chat-room peer that `client.exe` is built from |
| MFCKline | the `MFC` dialog flavour of the K-line client (login / register / log windows, OpenGL drawing) |
| WPFKline | a K-line application using _`C#`_ |
| TestUtils | test cases for the _KlineUtil_ interfaces |

Market
------

Prebuilt demo binaries kept as they were produced; the impact of one of them:

<img src="assets/impact.png" title="impact" height="80%" width="80%" align="middle" />
