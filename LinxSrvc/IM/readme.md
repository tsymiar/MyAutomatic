## KaiSocket

Cross-platform async messaging framework — the transport & pub/sub layer powering **[scadup](https://github.com/tsymiar/scadup)**.

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

```bash
cmake -B build && cmake --build build   # → libkaics.so
```

### Usage

```cpp
// Server
KaiSocket kai;
kai.Initialize(9999);
kai.registerCallback(hook_rcv);
kai.start();

// Client
kai.Initialize("127.0.0.1", 9999);
kai.registerCallback(hook_rcv);
kai.connect();

// Broker
kai.Broker();  // = registerCallback(proxyHook) + start()

// Pub/Sub
kai.Publisher("topic", "payload");
kai.Subscriber("topic", [](const auto& msg) { printf("%s\n", msg.data.body); });

// CLI
./kaics.exe -S | -C | -BK | -PB topic msg | -SS topic | -TF topic file
```

### Related

- **[scadup](https://github.com/tsymiar/scadup)** — higher-level library built on KaiSocket

```
scadup (sync engine / CLI)
  └── KaiSocket (async messaging / transport)  ← this repo
        └── epoll + sockets (Linux/macOS/Windows)
```
