# FileTransfer

macOS 文件传输应用 —— 底层 C++ 协议引擎 + 上层 SwiftUI 界面。

## 架构

```
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
                 │  Logger.h                │  FTF 协议
                 └────────────────-─────────┘
```

| 层 | 文件 | 职责 |
|----|------|------|
| **引擎** | `TransferEngine` | POSIX socket 服务器/客户端，FTF 协议编解码，多客户端并发接收 |
| **桥接** | `TransferBridge` | C API，将 C++ 类封装为不透明 `FT_Handle`，回调带 `void* userData` |
| **封装** | `TransferCore` | `@MainActor ObservableObject`，通过 `Unmanaged` 将 Swift 实例传入 C 回调 |
| **界面** | `ContentView` | 三标签页：Receive / Send / History，拖放文件，进度条 |
| **模型** | `TransferModels` | `TransferTask`（传输状态机）、`ReceivedFile`（接收记录）|
| **工具** | `UtilityExtensions` | `formatBytes()`、`timeFormatter` |

## 协议 (FTF)

- 魔数：`FTF\0`（4 字节）
- 头大小：64 字节（大端序）
- 命令：`1`=请求, `2`=响应, `3`=数据分片, `4`=完成, `5`=取消
- 分片：64 KB
- 默认端口：`8800`

与支持 FTF 协议的端完全兼容。

## 构建

```bash
cd LinxSrvc/mac/FileTransfer
swift run
```

或在 Xcode 中打开 `Package.swift` → 选择 `FileTransferMac` scheme → Run。

## 使用

### 接收文件（Server Mode）
1. 切换到 **Receive** 标签页
2. 确认本机 IP 和端口（默认 8800）
3. 点击 **Start Listening**
4. 对端设备连接到显示的 IP:Port 即可发送文件

### 发送文件（Client Mode）
1. 切换到 **Send** 标签页
2. 输入目标设备的 IP 和端口
3. 点击 **Connect**
4. 拖放文件到虚线区域，或点击 **Select Files…**

### 文件保存位置
接收到的文件保存到 `~/Downloads/FileTransfer/` 。可在菜单 **FileTransfer → Settings…**（⌘,）中修改。
