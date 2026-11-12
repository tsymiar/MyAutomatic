---
TITLE: FileTransfer
---

macOS 文件传输应用 —— 底层 C++ 协议引擎 + 上层 SwiftUI 界面。

- [架构](#架构)
- [协议 (FTF)](#协议-ftf)
- [构建](#构建)
- [使用](#使用)

## 架构

```text
                 ┌────────────────────────-─┐
                 │  SwiftUI Views           │
                 │  Views/ContentView.swift │
                 ├───────-──────────────────┤
                 │  @MainActor              │
                 │  Core/TransferCore.swift │
                 ├───────────-──────────────┤
        C Bridge │  include/TransferBridge.h│  extern "C"
                 │  TransferBridge.cpp      │
                 ├─────-────────────────────┤
                 │  TransferEngine.h / .cpp │  POSIX socket
                 │  CommLogger.h            │  FTF 协议
                 └────────────────-─────────┘
```

| 层 | 文件 | 职责 |
|----|------|------|
| **引擎** | `TransferEngine` | POSIX socket 服务器/客户端，FTF 协议编解码，多客户端并发接收 |
| **桥接** | `TransferBridge` | C API，封装 C++ 类为 `FT_Handle`，回调带 `void*` |
| **封装** | `TransferCore` | `@MainActor` 类，通过 `Unmanaged` 把 Swift 实例传给 C 回调 |
| **界面** | `ContentView` | 三标签页：Server / Client / History，拖放文件，进度条 |
| **模型** | `TransferModels` | `TransferTask`（传输状态机）、`ReceivedFile`（接收记录）|
| **工具** | `UtilityExtensions` | `formatBytes()`、`timeFormatter` |

## 协议 (FTF)

- 魔数：`'F','T',0xF0+version,0x00`（v1 → `0xF1`），版本不同不互通
- 头：64 字节，大端序
- 分片：256 KB
- 默认端口：`8800`

| 命令 | 含义 |
|----|------|
| 1 | 请求传输 |
| 2 | 响应：`event=1` 接受（`fileSize` 可为 0 的空文件）；`event=0` 且 `fileSize=0` 拒绝 |
| 3 | 数据分片 |
| 4 | 完成 |
| 5 | 取消 |
| 6 | 删除取消残留的半截文件 |

## 构建

要求 macOS 13+ / Swift 5.9（`Package.swift` 里两个 target：
`FileTransferCore` 静态库 → `FileTransferMac` 可执行）。

```bash
cd LinxSrvc/Mac/Transfer
swift build -c release   # 只编译
./build_app.sh test      # 编译 C++ 引擎 + CLI 测试程序（Linux 也可用）
./build_app.sh app       # macOS：打出 FileTransferMac.app 并做 ad-hoc 签名
./build_app.sh           # auto：macOS 走 app，Linux 走 test
./build_app.sh clean     # 清理 .build 与 .app
```

也可以用 Xcode 打开 `Package.swift` → 选 `FileTransferMac` scheme → Run。
注意裸二进制 / Xcode Run 不带 `.app` 的 `Info.plist`，本地网络权限声明
（`NSLocalNetworkUsageDescription`）只在 `build_app.sh app` 生成的 bundle 里，
因此访问 `192.168.x.x` 请优先用打包后的 `.app`。

> Linux 上**无法**编译 SwiftUI 目标（缺 SwiftUI / AppKit / Combine），但 C++ 引擎可完整编译链接：
> `./build_app.sh test` 产出 `.build/test/transfer`（server / send / pull / del 子命令）。

## 使用

### 接收文件

1. 切换到 **Server** 标签页
2. 确认本机 IP 和端口（默认 8800）
3. 点击 **Start Listening**
4. 对端设备连接到显示的 IP:Port 即可发送文件

### 发送文件

1. 切换到 **Client** 标签页
2. 输入目标设备的 IP 和端口
3. 点击 **Connect**
4. 拖放文件到虚线区域，或点击 **Select Files…**

### 文件写入位置

接收到的文件保存到 `~/Downloads/FileTransfer/`（默认，持久化于 UserDefaults）。文件名保持原名，
同名时在扩展名前追加 `-1`、`-2` 等序号，不覆盖已有文件；文件名过长会自动预留序号空间以符合 `NAME_MAX`。
保存目录可为任意形式（相对/绝对、带或不带结尾分隔符），引擎内部统一规范化。
修改入口：**Server 标签页右上角齿轮** → `Receive Location` + `Choose…`，选择后立即对正在监听的引擎生效。

### 设置项分工

| 入口 | 内容 |
|------|------|
| Server 页齿轮 popover | 指数退避开关（`Exponential backoff`，默认关闭=快速判死）、文件保存路径 |
| ⌘, 设置窗口 | 默认端口（预填 Server/Client 端口字段，改动即时生效，默认 `8800`）、是否显示日志控制台（默认开） |

### macOS「本地网络」权限（连不上时先看这里）

局域网传输需要 **本地网络（Local Network）** 权限：`localhost` 不受限，但访问 `192.168.x.x`
未授权时 `connect()` 会直接返回 `EHOSTUNREACH`（*No route to host*）。

- `Info.plist` 已声明 `NSLocalNetworkUsageDescription`（由 `build_app.sh` 生成），
  首次连接时系统会弹窗，**两端都要允许**；
- 若错过弹窗：系统设置 → 隐私与安全性 → 本地网络 → 勾选 FileTransfer；
- `.app` 用 ad-hoc 签名（`codesign --force --deep --sign -`），重打包后若仍被拒，
  再确认防火墙允许入站。

报错 errno 速查：`ECONNREFUSED`=端口没监听；静默丢包=poll 超时；`EHOSTUNREACH`=权限/链路问题。
