//
// Created by Shenyrion on 2025.
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

// 事件码 FT_EVT_* 只在 include/TransferBridge.h 定义一份（C 兼容，C++/Swift 共用）；
// 随 FileHeader.event 传输（占用 reserved[0]，线上格式向后兼容）。
#include "include/TransferBridge.h"

// 传输进度回调。event 取 FT_EVT_*；event==FT_EVT_NONE 时为常规进度刷新。
using ProgressCallback = std::function<void(uint64_t current, uint64_t total,
                                            const std::string& status, uint8_t event)>;

// 客户端会话结构 - 管理每个客户端的文件接收状态
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

// 客户端会话管理类
class ClientSessionMgr {
public:
    void addSession(int sock, const std::string& ip, unsigned short port);
    void removeSession(int sock);
    ClientSession* getSession(int sock);
    void forEachSession(const std::function<void(ClientSession&)>& func);
    void closeAllSockets();
    size_t size() const;

private:
    std::map<int, std::unique_ptr<ClientSession>> m_sessions;
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

    // 服务器模式
    int startServer(unsigned short port = DEFAULT_PORT);
    void closeServer();

    // 客户端模式
    int connectToServer(const std::string& ip, unsigned short port);
    void disconnect();

    // 请求取消当前传输（发送/拉取），在下一个分片边界生效。
    // 取消发送会向对端发 CMD_CANCEL，对端停止接收并按取消语义清理；
    // 取消拉取则本端直接停止落盘。
    void cancelTransfer();

    // 请求对端删除此前取消传输残留的半截文件（按原始文件名匹配）。
    // 对端只在收到 CMD_CANCEL 时记录的路径里找，没有记录则忽略。
    int deletePartialFile(const std::string& fileName);

    // 文件接收
    void setSavePath(const std::string& path);
    void setProgressCallback(ProgressCallback callback);

    // ENOTCONN 重试策略：true = 指数退避（最多 16 次、间隔翻倍、累计约 655s）；
    // false = 默认，3 次固定 20ms 宽限后判死关闭。可在 UI 里切换。
    void setBackoffEnabled(bool enabled);
    bool isBackoffEnabled() const { return m_backoffEnabled.load(); }

    // 文件发送
    int postLocalFile(const std::string& filePath);

    // 拉取模式：连接对端请求取回文件，阻塞直到接收完成或失败。
    // 对端只在其 m_savePath 下找同名普通文件；找不到回 fileSize=0，本端返回 -5。
    int requestFile(const std::string& ip, unsigned short port, const std::string& fileName);

    // 状态查询
    bool isConnected() const { return m_connected.load(); }
    bool isServerRunning() const { return m_serverRunning.load(); }
    int getClientCount() const { return (int)m_sessionMgr.size(); }

private:
    int createServerSocket(unsigned short port);
    int createClientSocket(const std::string& ip, unsigned short port);

    void fileServerProcess();
    void clientHandler(int sock, const std::string& clientIp, unsigned short clientPort);

    // 分片循环停止条件：push 用 m_connected，服务端代发（pull）用 session->active
    int sendSliceData(int sock, const std::string& filePath, uint64_t fileSize,
                      const std::atomic<bool>& runFlag);
    // 拉取模式的客户端接收循环：与服务端 CMD_DATA 相同的分片语义落盘
    int receivePullData(int sock, const std::string& fileName, uint64_t fileSize);
    int sendHeader(int sock, const void* data, size_t len);
    int sendHeader(int sock, const FileHeader& header);
    int recvHeader(int sock, FileHeader& header);

private:
    int m_serverSock;
    int m_clientSock;

    std::atomic<bool> m_serverRunning;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_backoffEnabled;  // ENOTCONN 是否走指数退避（默认 false）
    std::atomic<bool> m_cancelRequested; // 取消请求：发送/拉取在下一个分片边界生效

    // CMD_CANCEL 时记录的半截文件：原始文件名 -> 磁盘完整路径，等待对端 CMD_DELETE
    std::map<std::string, std::string> m_cancelledPartials;
    std::mutex m_partialMutex;

    std::thread m_serverThread;
    // 每个客户端一个异步任务。已结束的任务会在 accept 新连接时被回收，
    // 避免长时间运行的 server 持续堆积线程对象。
    std::vector<std::future<void>> m_clientFutures;

    std::mutex m_sendMutex;
    std::mutex m_clientThreadMutex;

    std::string m_savePath;
    std::string m_serverIp;
    unsigned short m_serverPort;

    ProgressCallback m_progressCallback;

    ClientSessionMgr m_sessionMgr;
};

#endif //DEVICE2DEVICE_TRANSFER_ENGINE_H
