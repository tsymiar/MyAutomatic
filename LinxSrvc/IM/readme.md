
Two **independent** things live in this directory — they share the folder and nothing else: no common
source, no common protocol, no common build target.

| Project | Sources | Produces |
| :-- | :-- | :-- |
| [IM](#im-chat-server-imcc) | `IM.cc` | `../bin/IM.exe` — a standalone chat-room server (+ its peer `../bin/client.exe`) |
| [KaiSocket](#kaisocket-messaging-library) | `KaiSocket.h` / `KaiSocket.cc` + `KaiTest.cxx` | the `kaics` library and the `../bin/kaics.exe` CLI |

---

## IM (chat server, `IM.cc`)

A self-contained chat server: register, login, exchange messages, manage group zones, and let the
server broker a peer-to-peer link. It does **not** use KaiSocket.

| Item | Value |
|------|-------|
| Source | `IM.cc` (~1800 lines, single translation unit) |
| Listen port | `argv[1]`, default `8877` (`DEFAULT_PORT`) |
| Concurrency | `fork()` per connection; `select()` based liveness probe |
| Shared state | System V shared memory online table, `IPC_KEY 0x520607`; after daemonizing it sets `prctl(PR_SET_NAME,"inst_mssg")` under `umask(077)` |
| Commands | `-m` dump the online table, `-x` reset it, console thread accepts `quit` / `kick` / `cls` |

### Architecture

```
                            IM.exe   (IM.cc — one translation unit)

  $ IM.exe 8877                             $ IM.exe -m │ -x
      │ port, DEFAULT_PORT = 8877               │ touch the online table only, exit
      ▼                                         ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │ main()                                                                 │
 │   fork(): child installs the SIGPIPE handler, prctl(PR_SET_NAME,       │
 │   "inst_mssg"), umask(077); afterwards setsid() + chdir("/")           │
 │   + close STDIN/STDOUT/STDERR  →  runs detached                        │
 └───────────────────────────────────┬────────────────────────────────────┘
                                     ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │ inst_mssg()                                     ┌────────────────────┐ │
 │   load_accnt() ← ./accounts                     │ commands() thread  │ │
 │      (file absent ⇒ seed user "iv9527"          │  quit │ kick │ cls │ │
 │       and the group zone "all")                 └────────────────────┘ │
 │   process-shared mutex (PTHREAD_PROCESS_SHARED)                        │
 │   socket → bind → listen, backlog 50                                   │
 │   SIGCHLD → func_waitpid()   (reaps exited children)                   │
 └───────────────────────────────────┬────────────────────────────────────┘
                                     │ accept() loop ──► fork() per connection
                                     ▼   (Windows: one thread that accept()s itself)
 ┌────────────────────────────────────────────────────────────────────────┐
 │ monitor(sock)      one child per peer — this is what client.exe hits   │
 │                                                                        │
 │   pre-auth   uiCmdMsg 0x00 ─► new_user()   + join_zone("all")          │
 │              uiCmdMsg 0x01 ─► user_auth()  + set_user_line()           │
 │   session    while (loggedIn) { 2-byte heartbeat every 30 s; dispatch} │
 └───────────────────────────────────┬────────────────────────────────────┘
                                     │ every command below reads / writes
                ┌────────────────────┴────────────────────┐
                ▼                                         ▼
 ┌──────────────────────────────┐        ┌──────────────────────────────────┐
 │ users[99] · zones[9]         │        │ shm ONLINE[30] ([6] on macOS)    │
 │ process-local arrays,        │        │ IPC_KEY 0x520607, reached through│
 │ persisted by save_accnt()    │        │ set_n_get_mem<T>() while holding │
 │ fwrite-ing raw structs into  │        │ the process-shared mutex trylock │
 │ ./accounts                   │        │ (shared by all the children)     │
 └──────────────────────────────┘        └──────────────────────────────────┘
```

### Session commands

Once logged in, `uiCmdMsg` selects one of these:

| uiCmdMsg | Command | What the server does |
|:--|:--|:--|
| `0x01` | `CHECK` | answers whether that account is already on-line |
| `0x02` | `USERNAME` | echoes the user name back with its signature |
| `0x03` | `LOGOUT` | `set_user_quit()` — frees the shm slot, tells the other side "`[%s] has logout`" |
| `0x04` | `PASSWD` | password change (the stored one must match first) |
| `0x05` | `ONLINE` | walks the shared-memory table and returns the online list |
| `0x06` | `P2P` | resolves the peer, answers with its IP/port, and forwards the `P2P` "hole digging" packet straight to that peer's socket |
| `0x07` | `NDT` | same lookup, but pushes a 32-byte random-tagged probe — the NAT-traversal twin of `P2P` |
| `0x08` | `EXEC` | `vfork()` + `execvp()` of an external program |
| `0x09` | `IMAGE` | still capture via `execvp("imgfilesnap.exe")` (or `raspistill` under `RASPI`) |
| `0x0a` | `ZONES` | active group list |
| `0x0b` | `MEMBERS` | members of a group |
| `0x0c` | `HOST_ZONE` | `host_zone()` — creates a group, or joins it when the name is taken |
| `0x0d` | `JOIN_ZONE` | `join_zone()` by group name |
| `0x0e` | `EXIT_ZONE` | `exit_zone()` — leaves the current group |
| `0x0f` | `ALL_ZONE` | `free_zone()` — disbands the group |

### Wire format

One fixed `USER` struct per request, one up-to-256-byte reply:

| Direction | Layout |
|-----------|--------|
| request | `head(1B)` \| `uiCmdMsg(1B)` \| `rtn[2]` \| `chk[4]` \| `usr[24]` \| `psw`│`TOKEN`│`peerIp[24]` \| `peer`│`port`│`sign`│`npsw`│`host`│`join`│`seek[24]` \| `peer_msg` \| `status[8]` |
| reply | echoed head bytes at 0, hex status code at 2, a `uint16` total length at 6, human-readable text from 8 |

The phase lives in `uiCmdMsg`: before login only `0x00` (register) and `0x01` (login) are handled — after
login the same byte is read as the command enum (`CHECK 0x01` … `ALL_ZONE 0x0f`), so `0x01` means *login*
first and *CHECK* afterwards. Anything else sent pre-auth gets a "Login / Register at first" reply.

### Build &amp; run

| Entry | Command | Output |
|-------|---------|--------|
| Makefile | `make IM` (it is also part of `make all` = `IM Kai client`) | `../bin/IM.exe` |
| Windows | the `IM` project (`IM.vcxproj`) of `WinNTKline/WinNTKline.sln` | `IM.exe` |

```bash
./IM.exe 8877    # base port
./IM.exe -m      # print the shared-memory online table
./IM.exe -x      # reset it
```

`make client` builds the chat-room peer `../bin/client.exe` from `../../WinNTKline/KlineUtil/IM/client.cc`
and `../../WinNTKline/IMclient/imclient.cpp`, so it only shows up when that sibling project is checked
out next to `LinxSrvc`.

### Caveats

- The account/group table is persisted by `fwrite`-ing the **raw struct** into `./accounts` — changing
  any struct field makes old files unreadable, and the layout is not portable across platforms.
- The `EXEC` / `IMAGE` message branches call `execvp("imgfilesnap.exe")` (or `raspistill` on
  Raspberry Pi); treat them as a command-execution surface and constrain the input.

---

## KaiSocket (messaging library)

Cross-platform async messaging framework — the transport &amp; pub/sub layer powering
**[scadup](https://github.com/tsymiar/scadup)**. Nothing in this half is shared with `IM.cc` above.

### Architecture

```
┌──────────────────────────────────────────────────────────┐
│                       KaiSocket                          │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐              │
│  │  SERVER  │   │  BROKER  │   │  CLIENT  │  Transport   │
│  │  start() │   │ Broker() │   │connect() │              │
│  └────┬─────┘   └────┬─────┘   └────┬─────┘              │
│       └───────┬──────┘              │                    │
│               ▼                     ▼                    │
│  ┌──────────────────────────────────────┐                │
│  │   Network: socket│IP│PORT│epoll      │    I/O layer   │
│  └────────────────┬─────────────────────┘                │
│                   │                                      │
│     ┌─────────────┼─────────────┐                        │
│     ▼             ▼             ▼                        │
│  ┌────────┐  ┌──────────┐  ┌───────────┐                 │
│  │PRODUCER│  │ CONSUMER │  │ SUBSCRIBE │     Messaging   │
│  └───┬────┘  └────┬─────┘  └────┬──────┘                 │
│      │            │             │                        │
│      └─────┬──────┘             │                        │
│            ▼                    │                        │
│    ┌──────────────┐             │                        │
│    │  m_msgQue    │◄────────────┘                        │
│    │  deque<Msg*> │                                      │
│    └──────────────┘                                      │
│                                                          │
│   Callbacks: KAI_SOCK_HOOK → each runs in own thread     │
└──────────────────────────────────────────────────────────┘

PUBLISH → PRODUCER → m_msgQue → CONSUMER → SUBSCRIBE
```

### Message Protocol

| Field | Layout |
|-------|--------|
| Header (48B, 4-aligned) | `rsv(1B)` \| `etag(4B)` \| `ssid(8B)` \| `text[32]` \| `size(4B)` |
| Payload (1-packed) | `stat[8]` \| `body[0..64K]` |

`ssid = PORT<<16 | socket<<8 | IP` | `etag = KaiRoles enum`

### Build

| Entry | Command | Output |
|-------|---------|--------|
| CMake (library + CLI) | `cmake -B build && cmake --build build` | shared library target `kaics` (`libkaics.so`) and the `kaics.exe` CLI |
| Makefile | `make Kai` (also part of `make all`) | `../bin/kaics.exe`, with `kaics.cfg` copied next to it |
| Windows | the `Scadup` project of `WinNTKline/WinNTKline.sln`, which compiles `../../LinxSrvc/IM/KaiSocket.cc` + `KaiTest.cxx` | the same CLI as a Windows target |

The endpoint comes from `kaics.cfg` (`IP=…`, default `127.0.0.1`; the port is hard-coded `9999`), read
from the working directory — only the Makefile copies that file, so keep a copy where you launch the
binary from.

### Usage

```cpp
// Server
KaiSocket kai;
kai.Initialize(9999);
kai.registerCallback(hook_recv);
kai.start();

// Client
kai.Initialize("127.0.0.1", 9999);
kai.registerCallback(hook_recv);
kai.connect();

// Broker
kai.Broker();  // = registerCallback(proxyHook) + start()

// Pub/Sub
kai.Publisher("topic", "payload");
kai.Subscriber("topic", [](const auto& msg) { printf("%s\n", msg.data.body); });

// CLI
./kaics.exe -S | -C | -BK | -PB topic msg | -SS topic | -TF topic file
```

### Caveats

- KaiSocket only uses real `epoll` when built with `USE_EPOLL` on Linux; otherwise it degrades to a
  blocking `accept` + `select()`/sleep loop.

### Related

- **[scadup](https://github.com/tsymiar/scadup)** — higher-level library built on KaiSocket

```
scadup (sync engine / CLI)
  └── KaiSocket (async messaging / transport)  ← this repo
        └── epoll + sockets (Linux/macOS/Windows)
```
