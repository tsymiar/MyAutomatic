//
// By Shenyrion, 2025.
//

#ifndef DEVICE2DEVICE_TRANSFER_ENGINE_H
#define DEVICE2DEVICE_TRANSFER_ENGINE_H

#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <fstream>
#include <memory>
#include <map>
#include <vector>
#include <future>

#ifndef LOG_TAG
#define LOG_TAG "TransferEngine"
#endif

#include "CommLogger.h"

// 版本写进魔数第 3 字节，版本不同的端在魔数校验阶段就被拒。
static constexpr uint8_t PROTOCOL_VERSION = 1;
static constexpr uint8_t kProtocolMagic[4] = { 'F', 'T', 0xF0 + PROTOCOL_VERSION, 0x00 };

// 文件传输协议头 (64字节), 大端序(网络字节序)
#pragma pack(push, 1)
struct FileHeader {
    uint8_t  magic[4];       // 魔数: 'F','T',0xF0+version,0x00
    uint8_t  version;        // 协议版本
    uint16_t cmd;            // 命令
    uint32_t fileNameLen;    // 文件名长度
    uint64_t fileSize;       // 文件总大小
    uint32_t chunkSize;      // 分片大小
    uint32_t chunkCount;     // 分片总数
    uint32_t currentChunk;   // 当前分片索引
    uint64_t transSize;      // 已传输大小
    uint8_t  event;          // 事件码 FT_EVT_*（CMD_COMPLETE/CANCEL 时携带）
    uint8_t  reserved[24];   // 保留（补齐到 64 字节）
};
#pragma pack(pop)

// 协议头必须 64 字节（字段 55 字节 + reserved 补齐）；改动字段会在此编译失败。
static_assert(sizeof(FileHeader) == 64, "FileHeader must be exactly 64 bytes");

// FT_EVT_* 仅在 TransferBridge.h 定义（C/C++/Swift 共用），经 FileHeader.event 的 reserved[0] 传输，线上格式兼容。
#include "include/TransferBridge.h"

// 传输进度回调。event 取 FT_EVT_*；event==FT_EVT_NONE 时为常规进度刷新。
using ProgressCallback = std::function<void(uint64_t current, uint64_t total,
    const std::string& status, uint8_t event)>;

// setter 与工作线程并发时用 shared_ptr 快照，避免 std::function 在被替换析构的同时被调用。
class ThreadSafeProgressCallback {
public:
    void set(ProgressCallback cb)
    {
        auto next = cb ? std::make_shared<ProgressCallback>(std::move(cb)) : nullptr;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cb = std::move(next);
    }
    explicit operator bool() const { return static_cast<bool>(snapshot()); }
    void operator()(uint64_t current, uint64_t total,
        const std::string& status, uint8_t event) const
    {
        auto cb = snapshot();
        if (cb && *cb) (*cb)(current, total, status, event);
    }
private:
    std::shared_ptr<ProgressCallback> snapshot() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cb;
    }
    mutable std::mutex m_mutex;
    std::shared_ptr<ProgressCallback> m_cb;
};

struct ClientSession {
    int sock;
    std::string clientIp;
    unsigned short clientPort;
    std::ofstream recvFile;           // 接收文件流
    std::string pendingFileName;      // 待接收文件名
    uint64_t pendingFileSize;         // 待接收文件大小
    uint64_t transSize;               // 已传输大小
    std::atomic<bool> active;         // 会话是否活跃
    std::string currentFilePath;      // 本次实际落盘的完整路径（完成后回传给 UI）
    std::vector<char> chunkBuf;       // 分片接收缓冲，跨分片复用避免每片重新分配

    ClientSession() : sock(-1), clientPort(0), pendingFileSize(0), transSize(0), active(false) {}
};

// 设备快照：短连接断开后仍保留最近一次连接记录，UI 才能稳定显示设备
struct ClientInfo {
    std::string ip;
    unsigned short port = 0;
    bool online = false;
    uint64_t lastSeenMs = 0;
    // 会话正在接收的文件与进度；file 为空表示空闲（以 recvFile 是否打开为准，完成/取消后即关闭）
    std::string transferringFile;
    uint64_t transferredBytes = 0;
    uint64_t totalBytes = 0;
};

class ClientSessionMgr {
public:
    void addSession(int sock, const std::string& ip, unsigned short port);
    void removeSession(int sock);
    ClientSession* getSession(int sock);
    void forEachSession(const std::function<void(ClientSession&)>& func);
    void closeAllSockets();
    size_t size() const;
    // 按最近连接时间倒序返回设备快照（在线与最近离线均包含）
    std::vector<ClientInfo> recentClients() const;
    // 快照条目数（m_recent 大小）；调用方按它分配快照缓冲，用会话数会截断离线条目
    size_t recentCount() const;

private:
    std::map<int, std::unique_ptr<ClientSession>> m_sessions;
    std::vector<ClientInfo> m_recent;
    mutable std::mutex m_mutex;
};

class TransferEngine {
public:
    static constexpr uint16_t CMD_REQUEST = 1;  // 请求文件传输
    static constexpr uint16_t CMD_RESPONSE = 2;  // 响应（接受/拒绝）
    static constexpr uint16_t CMD_DATA = 3;  // 数据分片
    static constexpr uint16_t CMD_COMPLETE = 4;  // 传输完成
    static constexpr uint16_t CMD_CANCEL = 5;  // 传输取消
    static constexpr uint16_t CMD_DELETE = 6;  // 删除对端取消传输残留的半截文件

    // CMD_RESPONSE.event 的接受标记：fileSize 不再兼任拒绝状态，空文件也能被接受。
    static constexpr uint8_t RESP_ACCEPT = 1;

    // 256KB：局域网下 64KB 分片会被逐片 RTT/ACK 限住吞吐；chunkSize 为 32 位，上限充足
    static constexpr uint32_t MAX_CHUNK_SIZE = 256 * 1024;
    static constexpr uint16_t DEFAULT_PORT = 8800;
    static constexpr int CONNECT_TIMEOUT_MS = 3000;    // 客户端 connect 超时
    static constexpr int RECV_POLL_TIMEOUT_MS = 5000;  // 服务端 poll 超时（避免 Nagle/RTT 导致文件名读取超时）
    static constexpr int RECV_RESP_TIMEOUT_MS = 5000;  // 客户端等待响应超时
    static constexpr int SEND_TIMEOUT_SEC = 30;        // 发送超时（对端一直不读时不会永久卡死）
    static constexpr uint32_t MAX_FILE_NAME_LEN = 1024; // 文件名长度上限（防恶意长度触发超大分配）

    TransferEngine();
    ~TransferEngine();

    int startServer(unsigned short port = DEFAULT_PORT);
    void closeServer();

    int connectToServer(const std::string& ip, unsigned short port);
    void disconnect();

    // 取消当前传输，下个分片边界生效：发送侧向对端发 CMD_CANCEL 令其清理；拉取侧本端停止落盘。
    void cancelTransfer();

    // 请对端删除取消残留的半截文件（按原名匹配）；只删 CMD_CANCEL 记录过的路径，无记录则忽略。
    int deletePartialFile(const std::string& fileName);

    void setSavePath(const std::string& path);
    void setProgressCallback(ProgressCallback callback);

    // ENOTCONN 重试：true=指数退避（≤16 次、翻倍、约 655s）；false=默认 3×20ms 宽限后关闭，UI 可切换。
    void setBackoffEnabled(bool enabled);
    bool isBackoffEnabled() const { return m_backoffEnabled.load(); }

    int postLocalFile(const std::string& filePath);

    // 拉取模式：连对端取回文件并阻塞至完成；对端仅在 m_savePath 找同名普通文件，找不到本端返回 -5。
    int requestFile(const std::string& ip, unsigned short port, const std::string& fileName);

    bool isConnected() const { return m_connected.load(); }
    bool isServerRunning() const { return m_serverRunning.load(); }
    // 与 getClients() 同源：快照条数（含离线设备），而非当前活跃会话数
    int getClientCount() const { return (int)m_sessionMgr.recentCount(); }
    std::vector<ClientInfo> getClients() const { return m_sessionMgr.recentClients(); }

private:
    int createServerSocket(unsigned short port);
    int createClientSocket(const std::string& ip, unsigned short port);

    void fileServerProcess();
    void clientHandler(int sock, const std::string& clientIp, unsigned short clientPort);

    // 停止条件：push 用 m_connected，服务端代发用 session->active；cancelFlag 按传输独立，代发传 nullptr 不消费客户端取消。
    int sendSliceData(int sock, const std::string& filePath, uint64_t fileSize,
        const std::atomic<bool>& runFlag,
        std::shared_ptr<std::atomic<bool>> cancelFlag);
    // 拉取模式的客户端接收循环：与服务端 CMD_DATA 相同的分片语义落盘
    int receivePullData(int sock, const std::string& fileName, uint64_t fileSize,
        std::shared_ptr<std::atomic<bool>> cancelFlag);
    // 为一次客户端传输创建独立取消标志；weak_ptr 不延寿，传输结束即自动过期。
    std::shared_ptr<std::atomic<bool>> beginTransfer();
    int sendHeader(int sock, const void* data, size_t len);
    int sendHeader(int sock, const FileHeader& header);
    int recvHeader(int sock, FileHeader& header);

    std::string savePath() const;

private:
    int m_serverSock;
    std::atomic<int> m_clientSock;

    std::atomic<bool> m_serverRunning;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_backoffEnabled;  // ENOTCONN 是否走指数退避（默认 false）

    // 每次客户端传输一个独立标志，避免服务端并发代发互相消费，也避免新传输清掉刚发出的取消。
    std::mutex m_cancelMutex;
    std::weak_ptr<std::atomic<bool>> m_activeCancel;

    // CMD_CANCEL 时记录的半截文件：原始文件名 -> 磁盘完整路径，等待对端 CMD_DELETE
    std::map<std::string, std::string> m_cancelledPartials;
    std::mutex m_partialMutex;

    std::thread m_serverThread;
    // 每客户端一个异步任务；结束的任务在 accept 新连接时回收，避免长期运行的 server 堆积线程对象。
    std::vector<std::future<void>> m_clientFutures;

    std::mutex m_sendMutex;
    std::mutex m_clientThreadMutex;
    mutable std::mutex m_stateMutex;   // 保护 m_savePath / m_serverIp / m_serverPort / 回调

    std::string m_savePath;
    std::string m_serverIp;
    unsigned short m_serverPort;

    ThreadSafeProgressCallback m_progressCallback;

    ClientSessionMgr m_sessionMgr;
};

#endif //DEVICE2DEVICE_TRANSFER_ENGINE_H
