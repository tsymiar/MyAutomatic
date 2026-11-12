//
// By Shenyrion, 2025.
//

#include "TransferEngine.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <algorithm>
#include <memory>
#include <vector>
#include <map>
#include <ctime>

// 由 TransferBridge.cpp 指向 Swift 控制台
std::atomic<LogOutputFunc> g_logOutputFunc{ nullptr };

static inline void fillHeader(FileHeader& h, uint16_t cmd)
{
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, kProtocolMagic, sizeof(kProtocolMagic));
    h.version = PROTOCOL_VERSION;
    h.cmd = cmd;
}

// 字节序转换：协议头统一按大端(网络字节序)传输
static uint64_t hton64(uint64_t v)
{
    uint32_t hi = htonl((uint32_t)(v >> 32));
    uint32_t lo = htonl((uint32_t)(v & 0xFFFFFFFFu));
    return ((uint64_t)lo << 32) | (uint64_t)hi;
}

static void headerToNet(FileHeader& h)
{
    h.cmd = htons(h.cmd);
    h.fileNameLen = htonl(h.fileNameLen);
    h.fileSize = hton64(h.fileSize);
    h.chunkSize = htonl(h.chunkSize);
    h.chunkCount = htonl(h.chunkCount);
    h.currentChunk = htonl(h.currentChunk);
    h.transSize = hton64(h.transSize);
}

/// 转换是对称的，网络序 -> 主机序复用同一个函数。
static void headerToHost(FileHeader& h) { headerToNet(h); }

// 文件名安全化：剥掉路径成分，防 "../../.ssh/..." 式路径遍历
static std::string sanitizeFileName(const std::string& name)
{
    size_t pos = name.find_last_of("/\\");
    std::string base = (pos != std::string::npos) ? name.substr(pos + 1) : name;

    std::string out;
    out.reserve(base.size());
    for (unsigned char c : base) {
        if (c < 32 || c == 127 || c == '/' || c == '\\' || c == ':') {
            out += '_';
        } else {
            out += (char)c;
        }
    }
    if (out.empty() || out == "." || out == "..") {
        out = "received_file";
    }
    if (out.size() > 255) {
        out.resize(255);
    }
    return out;
}

// 目录任意形式（空、相对、绝对、带或不带结尾分隔符），统一补 '/' 结尾
static std::string normalizeDir(const std::string& dir)
{
    std::string d = dir.empty() ? "./" : dir;
    if (d.back() != '/' && d.back() != '\\') {
        d += '/';
    }
    return d;
}

// 保留原名，同名时在扩展名前追加 -1/-2；返回的候选（含首个）均已确认未被占用。
static std::string uniqueSavePath(const std::string& path, const std::string& name)
{
    const std::string root = normalizeDir(path);
    std::string base = sanitizeFileName(name);

    // 预留 -N 序号空间，确保拼接后不超过 NAME_MAX(255)。
    constexpr size_t kNameMax = 255;
    constexpr size_t kSuffixMax = 12;
    if (base.size() > kNameMax - kSuffixMax) {
        base.resize(kNameMax - kSuffixMax);
    }
    std::string stem = base, ext;
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos && dot > 0) {
        stem = base.substr(0, dot);
        ext = base.substr(dot);
    }

    std::string candidate = root + base;
    struct stat st {};
    for (int i = 1; stat(candidate.c_str(), &st) == 0; ++i) {
        candidate = root + stem + "-" + std::to_string(i) + ext;
    }
    return candidate;
}

// sendAll：循环 send 直到全部发出（send 不保证一次发完）。0 成功，-1 失败
static int sendAll(int sock, const void* buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
#ifdef MSG_NOSIGNAL
        ssize_t ret = send(sock, (const char*)buf + total, len - total, MSG_NOSIGNAL);
#else
        ssize_t ret = send(sock, (const char*)buf + total, len - total, 0);
#endif
        if (ret < 0) {
            if (errno == EINTR) {
                continue;  // 被信号中断
            }
            // EAGAIN = SO_SNDTIMEO 到期（对端不读），必须报错返回，continue 会忙等卡死
            return -1;
        }
        if (ret == 0) {
            return -1;  // send 返回 0 表示连接关闭
        }
        total += (size_t)ret;
    }
    return 0;
}

// recvAll：循环 recv 直到收满（SO_RCVTIMEO 建连时已设）；返回 >0 成功 / 0 EOF / -1 超时可重试 / -2 致命错误（上层短宽限后判死）。
static int recvAll(int sock, void* buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t r = recv(sock, (char*)buf + total, len - total, 0);
        if (r > 0) {
            total += (size_t)r;
            continue;
        }
        if (r == 0) {
            return 0;  // EOF
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;  // SO_RCVTIMEO 超时
        }
        if (errno == EINTR) {
            continue;
        }
        // ENOTCONN: 对端已 abort/连接已死（非瞬时），返回 -2 交上层判死关闭。
        [[maybe_unused]] int savedErrno = errno;
        int soErr = 0; socklen_t soLen = sizeof(soErr);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &soErr, &soLen);
        LOG_WRN("recvAll: recv errno=%d/%s, SO_ERROR=%d/%s",
            savedErrno, strerror(savedErrno), soErr, soErr ? strerror(soErr) : "(none)");
        return -2;
    }
    return (int)total;
}

static uint64_t epochMillis()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void ClientSessionMgr::addSession(int sock, const std::string& ip, unsigned short port)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unique_ptr<ClientSession> session(new ClientSession());
    session->sock = sock;
    session->clientIp = ip;
    session->clientPort = port;
    session->active = true;
    m_sessions[sock] = std::move(session);

    auto it = std::find_if(m_recent.begin(), m_recent.end(),
        [&ip](const ClientInfo& c) { return c.ip == ip; });
    if (it == m_recent.end()) {
        // 只保留最近 20 台设备，淘汰最久未连接的条目
        if (m_recent.size() >= 20) {
            auto oldest = std::min_element(m_recent.begin(), m_recent.end(),
                [](const ClientInfo& a, const ClientInfo& b) {
                    return a.lastSeenMs < b.lastSeenMs;
                });
            m_recent.erase(oldest);
        }
        ClientInfo info;
        info.ip = ip;
        info.port = port;
        info.online = true;
        info.lastSeenMs = epochMillis();
        m_recent.push_back(info);
    } else {
        it->port = port;
        it->online = true;
        it->lastSeenMs = epochMillis();
    }
}

void ClientSessionMgr::removeSession(int sock)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sock);
    if (it != m_sessions.end()) {
        if (it->second && it->second->recvFile.is_open()) {
            it->second->recvFile.close();
        }
        m_sessions.erase(it);
    }
    // 同一 IP 可能有多个并发会话，按剩余会话重新计算在线状态
    for (auto& c : m_recent) {
        c.online = std::any_of(m_sessions.begin(), m_sessions.end(),
            [&c](const std::pair<const int, std::unique_ptr<ClientSession>>& pair) {
                return pair.second && pair.second->clientIp == c.ip;
            });
    }
}

ClientSession* ClientSessionMgr::getSession(int sock)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sock);
    if (it != m_sessions.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ClientSessionMgr::forEachSession(const std::function<void(ClientSession&)>& func)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_sessions) {
        if (pair.second) {
            func(*pair.second);
        }
    }
}

void ClientSessionMgr::closeAllSockets()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_sessions) {
        if (pair.second && pair.second->sock >= 0) {
            // 仅 shutdown 唤醒 recv；close + removeSession 由 clientHandler 独占，避免重复 close 误关复用 fd。
            LOG_INF("closeAllSockets: waking client %s:%d (fd=%d)",
                pair.second->clientIp.c_str(), pair.second->clientPort, pair.second->sock);
            shutdown(pair.second->sock, SHUT_RDWR);
        }
    }
}

size_t ClientSessionMgr::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.size();
}

std::vector<ClientInfo> ClientSessionMgr::recentClients() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ClientInfo> out = m_recent;
    // 在线会话补充实时传输状态：recvFile 打开中才算正在接收（完成/取消后即关闭）
    for (auto& c : out) {
        for (auto& pair : m_sessions) {
            const auto& s = pair.second;
            if (s && s->clientIp == c.ip && s->recvFile.is_open()
                && !s->pendingFileName.empty()) {
                c.transferringFile = s->pendingFileName;
                c.transferredBytes = s->transSize;
                c.totalBytes = s->pendingFileSize;
                break;
            }
        }
    }
    std::sort(out.begin(), out.end(), [](const ClientInfo& a, const ClientInfo& b) {
        return a.lastSeenMs > b.lastSeenMs;
        });
    return out;
}

size_t ClientSessionMgr::recentCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recent.size();
}

TransferEngine::TransferEngine()
    : m_serverSock(-1)
    , m_clientSock(-1)
    , m_serverRunning(false)
    , m_connected(false)
    , m_backoffEnabled(false)
    , m_serverPort(0)
{}

TransferEngine::~TransferEngine()
{
    closeServer();
    disconnect();
}

void TransferEngine::setSavePath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_savePath = path;
}

void TransferEngine::setProgressCallback(ProgressCallback callback)
{
    m_progressCallback.set(std::move(callback));
}

std::string TransferEngine::savePath() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_savePath;
}

void TransferEngine::setBackoffEnabled(bool enabled)
{
    m_backoffEnabled.store(enabled);
}

int TransferEngine::createServerSocket(unsigned short port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERR("socket() failed: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
#ifdef SO_NOSIGPIPE
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR("bind() failed: %s", strerror(errno));
        close(sock);
        return -2;
    }

    if (listen(sock, 5) < 0) {
        LOG_ERR("listen() failed: %s", strerror(errno));
        close(sock);
        return -3;
    }

    LOG_INF("TransferEngine server listening on port %d", port);
    return sock;
}

int TransferEngine::createClientSocket(const std::string& ip, unsigned short port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERR("socket() failed: %s", strerror(errno));
        return -1;
    }

    // Keepalive 尽早发现断连；禁用 Nagle 降低发送延迟。
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
#ifdef SO_NOSIGPIPE
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

    // 发送超时：对端一直不读时 send() 会返回 EAGAIN 而不是永久阻塞
    struct timeval sndTv;
    sndTv.tv_sec = SEND_TIMEOUT_SEC;
    sndTv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sndTv, sizeof(sndTv));

    // 接收超时（等待服务端 CMD_RESPONSE 等场景），建连时设一次
    struct timeval rcvTv;
    rcvTv.tv_sec = RECV_RESP_TIMEOUT_MS / 1000;
    rcvTv.tv_usec = (RECV_RESP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &rcvTv, sizeof(rcvTv));

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        struct addrinfo hints {}, * res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int ga = getaddrinfo(ip.c_str(), nullptr, &hints, &res);
        if (ga != 0 || res == nullptr) {
            LOG_ERR("getaddrinfo() failed: %s -> %s", ip.c_str(), gai_strerror(ga));
            close(sock);
            return -2;
        }
        memcpy(&addr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(addr.sin_addr));
        freeaddrinfo(res);
    }

    // 阻塞 connect() 在 macOS 可能挂 75s+；改用 O_NONBLOCK → connect() → poll(POLLOUT) → 还原。
    int origFlags = fcntl(sock, F_GETFL, 0);
    if (origFlags < 0) {
        LOG_ERR("fcntl(F_GETFL) failed: %s", strerror(errno));
        close(sock);
        return -3;
    }
    fcntl(sock, F_SETFL, origFlags | O_NONBLOCK);

    int ret = connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        LOG_ERR("connect() to %s:%u immediate fail: %s", ip.c_str(), port, strerror(errno));
        close(sock);
        return -3;
    }

    if (ret < 0) {
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLOUT;

        int pr = poll(&pfd, 1, CONNECT_TIMEOUT_MS);
        if (pr == 0) {
            LOG_ERR("connect() to %s:%u timed out after %d ms",
                ip.c_str(), port, CONNECT_TIMEOUT_MS);
            close(sock);
            return -3;
        }
        if (pr < 0) {
            LOG_ERR("poll() failed: %s", strerror(errno));
            close(sock);
            return -3;
        }

        int sockErr = 0;
        socklen_t len = sizeof(sockErr);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErr, &len) < 0 || sockErr != 0) {
            LOG_ERR("connect() to %s:%u failed: %s",
                ip.c_str(), port, sockErr ? strerror(sockErr) : "getsockopt error");
            close(sock);
            return -3;
        }
    }

    fcntl(sock, F_SETFL, origFlags);

    LOG_INF("TransferEngine connected to %s:%u", ip.c_str(), port);
    return sock;
}

int TransferEngine::startServer(unsigned short port)
{
    if (m_serverRunning.load()) {
        LOG_WRN("TransferEngine server already running");
        return 0;
    }

    m_serverSock = createServerSocket(port);
    if (m_serverSock < 0) {
        return m_serverSock;
    }

    m_serverRunning.store(true);

    m_serverThread = std::thread(&TransferEngine::fileServerProcess, this);
    return 0;
}

void TransferEngine::closeServer()
{
    m_serverRunning.store(false);

    m_sessionMgr.closeAllSockets();

    if (m_serverSock >= 0) {
        shutdown(m_serverSock, SHUT_RDWR);
        close(m_serverSock);
        m_serverSock = -1;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    // 必须等所有 clientHandler 退出再析构，否则会访问已释放的 this
    {
        std::lock_guard<std::mutex> lock(m_clientThreadMutex);
        for (std::future<void>& f : m_clientFutures) {
            if (f.valid()) {
                f.wait();
            }
        }
        m_clientFutures.clear();
    }

    LOG_INF("TransferEngine Server Exit");
}

int TransferEngine::connectToServer(const std::string& ip, unsigned short port)
{
    if (m_connected.load()) {
        disconnect();
    }

    m_clientSock = createClientSocket(ip, port);
    if (m_clientSock < 0) {
        return m_clientSock;
    }

    m_serverIp = ip;
    m_serverPort = port;
    m_connected.store(true);
    return 0;
}

void TransferEngine::disconnect()
{
    m_connected.store(false);

    // 关 fd 须持发送锁（等 sendAll 结束，防 fd 复用误发），并先 shutdown 唤醒阻塞的 recv。
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (m_clientSock >= 0) {
            LOG_INF("TransferEngine disconnected from %s:%d (fd=%d)",
                m_serverIp.c_str(), m_serverPort, (int)m_clientSock);
            shutdown(m_clientSock, SHUT_RDWR);
            close(m_clientSock);
            m_clientSock = -1;
        }
    }
}

void TransferEngine::cancelTransfer()
{
    std::shared_ptr<std::atomic<bool>> flag;
    {
        std::lock_guard<std::mutex> lock(m_cancelMutex);
        flag = m_activeCancel.lock();
    }
    if (flag) {
        flag->store(true);
        LOG_INF("cancel requested — takes effect at next chunk boundary");
    } else {
        LOG_WRN("cancel requested but no active client transfer");
    }
}

std::shared_ptr<std::atomic<bool>> TransferEngine::beginTransfer()
{
    auto flag = std::make_shared<std::atomic<bool>>(false);
    std::lock_guard<std::mutex> lock(m_cancelMutex);
    m_activeCancel = flag;
    return flag;
}

int TransferEngine::deletePartialFile(const std::string& fileName)
{
    if (fileName.empty()) {
        LOG_ERR("deletePartialFile: empty fileName");
        return -1;
    }
    if (!m_connected.load()) {
        LOG_ERR("deletePartialFile: not connected");
        return -2;
    }

    // header + filename 持锁按序发出（与服务端解析约定一致）
    FileHeader header{ };
    fillHeader(header, CMD_DELETE);
    header.fileNameLen = (uint32_t)fileName.size();

    FileHeader netHeader = header;
    headerToNet(netHeader);
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (sendAll(m_clientSock, &netHeader, sizeof(netHeader)) < 0 ||
            sendAll(m_clientSock, fileName.c_str(), fileName.size()) < 0) {
            LOG_ERR("sendAll(delete request) failed: %s", strerror(errno));
            return -3;
        }
    }
    LOG_INF("deletePartialFile: requested deletion of \"%s\" on peer", fileName.c_str());
    return 0;
}

void TransferEngine::fileServerProcess()
{
    struct sockaddr_in clientAddr {};
    socklen_t addrLen = sizeof(clientAddr);

    while (m_serverRunning.load()) {
        int clientSock = accept(m_serverSock, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);
        if (clientSock < 0) {
            if (m_serverRunning.load()) {
                LOG_ERR("accept() failed: %s", strerror(errno));
            }
            break;
        }

        char ipStr[INET_ADDRSTRLEN];
        unsigned short clientPort = ntohs(clientAddr.sin_port);
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

        // 非阻塞 MSG_PEEK 探测并丢弃已 abort 的连接；EAGAIN=空闲正常，不消耗已到达的数据。
        int probeFlags = fcntl(clientSock, F_GETFL, 0);
        if (probeFlags >= 0) {
            fcntl(clientSock, F_SETFL, probeFlags | O_NONBLOCK);
        }
        char probeByte = 0;
        ssize_t probe = recv(clientSock, &probeByte, 1, MSG_PEEK);
        int probeErrno = errno;  // 必须在还原 flags 前快照，后续 syscall 会覆盖 errno
        if (probeFlags >= 0) {
            fcntl(clientSock, F_SETFL, probeFlags);
        }
        if (probe == 0) {
            LOG_WRN("client %s:%d already closed (EOF) — dropping fd=%d", ipStr, clientPort, clientSock);
            close(clientSock);
            continue;
        }
        if (probe < 0 && probeErrno != EAGAIN && probeErrno != EWOULDBLOCK && probeErrno != EINTR) {
            LOG_WRN("client %s:%d dead on arrival (errno=%d/%s) — dropping fd=%d",
                ipStr, clientPort, probeErrno, strerror(probeErrno), clientSock);
            close(clientSock);
            continue;
        }

        // 禁用 Nagle 确保 header+filename 小包立即发出不分段
        int opt = 1;
        setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
#ifdef SO_NOSIGPIPE
        setsockopt(clientSock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

        // 发送超时：对端一直不读时不至于永久卡住
        struct timeval sndTv;
        sndTv.tv_sec = SEND_TIMEOUT_SEC;
        sndTv.tv_usec = 0;
        setsockopt(clientSock, SOL_SOCKET, SO_SNDTIMEO, &sndTv, sizeof(sndTv));

        // 接收超时只在这里设一次，recvAll 不必每个分片都重设
        struct timeval rcvTv;
        rcvTv.tv_sec = RECV_POLL_TIMEOUT_MS / 1000;
        rcvTv.tv_usec = (RECV_POLL_TIMEOUT_MS % 1000) * 1000;
        setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &rcvTv, sizeof(rcvTv));

        std::lock_guard<std::mutex> lock(m_clientThreadMutex);
        // 先回收已结束的任务，避免长跑时线程对象持续堆积
        m_clientFutures.erase(
            std::remove_if(m_clientFutures.begin(), m_clientFutures.end(),
                [](std::future<void>& f) {
                    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }),
            m_clientFutures.end());
        m_clientFutures.push_back(std::async(std::launch::async,
            &TransferEngine::clientHandler, this, clientSock, std::string(ipStr), clientPort));
    }
}

void TransferEngine::clientHandler(int sock, const std::string& clientIp, unsigned short clientPort)
{
    LOG_INF("clientHandler START fd=%d from %s:%u", sock, clientIp.c_str(), clientPort);

    // 确保 socket 为阻塞模式（accept 理论上已继承，显式设置消除平台差异）
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERR("clientHandler fd=%d fcntl(F_GETFL) failed: %d/%s — fd already invalid, exiting",
            sock, errno, strerror(errno));
        close(sock);
        m_sessionMgr.removeSession(sock);
        return;
    }
    if (flags & O_NONBLOCK) {
        LOG_WRN("clientHandler fd=%d was O_NONBLOCK, forcing blocking mode", sock);
        fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }

    m_sessionMgr.addSession(sock, clientIp, clientPort);
    ClientSession* session = m_sessionMgr.getSession(sock);
    if (!session) {
        LOG_ERR("clientHandler fd=%d getSession returned null — closing", sock);
        close(sock);
        return;
    }

    if (m_progressCallback) {
        m_progressCallback(0, 0, "Client connected from " + clientIp + ":" + std::to_string(clientPort),
            FT_EVT_SESSION_OPENED);
    }

    std::string statusPrefix = "[" + clientIp + ":" + std::to_string(clientPort) + "] ";

    // ENOTCONN/EBADF：连接已死且重试不自愈，默认短宽限后判死；指数退避久占线程，仅作 UI 可选项。
    static constexpr int ENOTCONN_RETRY_MAX = 3;
    static constexpr int ENOTCONN_RETRY_DELAY_US = 20000;
    static constexpr int BACKOFF_RETRY_MAX = 16;
    static constexpr int BACKOFF_BASE_DELAY_US = 10000;    // 指数退避：起始 10ms，逐次翻倍
    int enotconnRetries = 0;
    int lastLoggedPct = -1;
    int lastProgressPct = -1;   // 进度回调按 1% 步进去重，避免逐分片刷新 UI
    auto lastSpeedTime = std::chrono::steady_clock::now();
    uint64_t lastSpeedBytes = 0;

    while (m_serverRunning.load() && session->active.load()) {
        FileHeader header{ };
        int ret = recvAll(sock, &header, sizeof(header));
        if (ret <= 0) {
            if (ret == -1) {
                // 空闲连接每 5s 到此一次，仅 DEBUG；回到 while 复查 m_serverRunning
                LOG_DBG("%s recv header timeout (%dms) — retry", statusPrefix.c_str(), RECV_POLL_TIMEOUT_MS);
                enotconnRetries = 0;  // 超时后重置 ENOTCONN 计数器
                continue;
            }
            if (ret == -2) {
                const bool backoff = m_backoffEnabled.load();
                const int retryMax = backoff ? BACKOFF_RETRY_MAX : ENOTCONN_RETRY_MAX;
                if (enotconnRetries < retryMax) {
                    ++enotconnRetries;
                    long delayUs = backoff
                        ? (long)(BACKOFF_BASE_DELAY_US * (1LL << (enotconnRetries - 1)))
                        : (long)ENOTCONN_RETRY_DELAY_US;
                    LOG_WRN("%s recv header ENOTCONN/EBADF, retry %d/%d after %ldus (backoff=%d)",
                        statusPrefix.c_str(), enotconnRetries, retryMax, delayUs, (int)backoff);
                    // tv_nsec 必须 < 1e9，退避后期必须把整秒拆到 tv_sec
                    timespec nap;
                    nap.tv_sec = (time_t)(delayUs / 1000000);
                    nap.tv_nsec = (long)((delayUs % 1000000) * 1000);
                    nanosleep(&nap, nullptr);
                    continue;
                }
                // 宽限/退避用尽：连接确已死亡，按对端断开处理
                LOG_WRN("%s recv header ENOTCONN persists after %d retries — peer aborted, closing (fd=%d)",
                    statusPrefix.c_str(), retryMax, sock);
                break;
            }
            if (ret == 0) {
                LOG_INF("%s recv header: EOF — client disconnected (fd=%d)",
                    statusPrefix.c_str(), sock);
            } else {
                // 详细错误已由 recvAll 内部记录，或 ENOTCONN 重试耗尽
                LOG_WRN("%s recv header failed: ret=%d (fd=%d) — see recvAll log above",
                    statusPrefix.c_str(), ret, sock);
            }
            break;
        }
        enotconnRetries = 0;  // 成功收到数据，重置计数器

        headerToHost(header);

        if (memcmp(header.magic, kProtocolMagic, sizeof(kProtocolMagic)) != 0) {
            LOG_ERR("%s invalid magic number: 0x%08x, expect 0x%08x, client disconnected", statusPrefix.c_str(), *reinterpret_cast<uint32_t*>(header.magic), *reinterpret_cast<const uint32_t*>(kProtocolMagic));
            break;
        }

        // CMD_DATA 逐片打日志会刷屏，进度在分支内按 1% 步进打印
        if (header.cmd != CMD_DATA) {
            LOG_INF("%s Received command: %d", statusPrefix.c_str(), header.cmd);
        }
        switch (header.cmd) {
        case CMD_REQUEST: {
            // fileNameLen 必须校验：伪造超大长度会打爆内存（DoS）
            if (header.fileNameLen == 0 || header.fileNameLen > MAX_FILE_NAME_LEN) {
                LOG_ERR("%s invalid fileNameLen=%u (max %u) — closing",
                    statusPrefix.c_str(), header.fileNameLen, MAX_FILE_NAME_LEN);
                session->active.store(false);
                break;
            }

            std::string fileName(header.fileNameLen, '\0');
            int r = recvAll(sock, &fileName[0], header.fileNameLen);
            if (r <= 0) {
                if (r == 0) {
                    LOG_INF("%s client disconnected while reading filename", statusPrefix.c_str());
                } else {
                    LOG_ERR("%s recv filename timeout — protocol desync, closing", statusPrefix.c_str());
                }
                session->active.store(false);
                break;
            }

            // 剥路径成分防路径遍历
            std::string safeName = sanitizeFileName(fileName);
            LOG_INF("%s TransferEngine incoming request: %s (%llu bytes)",
                statusPrefix.c_str(), safeName.c_str(), (unsigned long long)header.fileSize);

            // 拉取模式：按 event 码区分 push/pull（二者 fileSize 都可能为 0）
            if (header.event == FT_EVT_REQUEST_PULL) {
                // safeName 已剥掉路径成分，只允许取 m_savePath 下的普通文件
                std::string filePath = normalizeDir(savePath()) + safeName;

                FileHeader resp{ };
                fillHeader(resp, CMD_RESPONSE);
                struct stat st {};
                if (stat(filePath.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
                    LOG_WRN("%s pull request rejected: %s not found (fd=%d)",
                        statusPrefix.c_str(), filePath.c_str(), sock);
                    if (sendHeader(sock, resp) < 0) {
                        LOG_ERR("%s sendHeader(pull reject) failed — closing", statusPrefix.c_str());
                        session->active.store(false);
                    }
                    break;
                }

                resp.event = RESP_ACCEPT;
                resp.fileSize = st.st_size;
                if (sendHeader(sock, resp) < 0) {
                    LOG_ERR("%s sendHeader(pull accept) failed — closing", statusPrefix.c_str());
                    session->active.store(false);
                    break;
                }

                LOG_INF("%s serving pull request: %s (%llu bytes)",
                    statusPrefix.c_str(), filePath.c_str(), (unsigned long long)st.st_size);
                // 服务端代发不响应客户端的取消请求：runFlag 用 session->active
                sendSliceData(sock, filePath, st.st_size, session->active, nullptr);
                break;
            }

            // push 模式：对端推文件过来
            session->pendingFileName = safeName;
            session->pendingFileSize = header.fileSize;
            session->transSize = 0;
            lastLoggedPct = -1;  // 新文件从 0% 重新计步
            lastSpeedTime = std::chrono::steady_clock::now();
            lastSpeedBytes = 0;

            FileHeader response{ };
            fillHeader(response, CMD_RESPONSE);
            response.fileSize = header.fileSize;
            if (sendHeader(sock, response) < 0) {
                LOG_ERR("%s sendHeader(response) failed — closing", statusPrefix.c_str());
                session->active.store(false);
                break;
            }

            if (m_progressCallback) {
                m_progressCallback(0, header.fileSize, statusPrefix + "Receiving: " + fileName,
                    FT_EVT_RECEIVE_START);
            }
            break;
        }
        case CMD_DATA: {
            // 校验在落盘前完成：拒绝超大分片可避免按伪造长度分配 GB 级缓冲。
            if (header.chunkSize == 0 || header.chunkSize > MAX_CHUNK_SIZE
                || header.currentChunk >= header.chunkCount
                || header.fileSize != session->pendingFileSize
                || session->pendingFileName.empty()) {
                LOG_ERR("%s invalid chunk meta: chunk %u/%u size=%u fileSize=%llu — closing",
                    statusPrefix.c_str(), header.currentChunk, header.chunkCount,
                    header.chunkSize, (unsigned long long)header.fileSize);
                session->active.store(false);
                break;
            }

            uint64_t offset = (uint64_t)header.currentChunk * header.chunkSize;
            // offset 与已收字节数不符 = 协议错位，继续写会写坏文件，判死
            if (offset != session->transSize) {
                LOG_ERR("%s chunk out of order before open: chunk %u offset=%llu but received %llu — closing",
                    statusPrefix.c_str(), header.currentChunk,
                    (unsigned long long)offset, (unsigned long long)session->transSize);
                session->active.store(false);
                break;
            }

            if (header.currentChunk == 0 && session->recvFile.is_open()) {
                session->recvFile.close();
            }

            if (!session->recvFile.is_open()) {
                std::string filePath = uniqueSavePath(savePath(), session->pendingFileName);

                session->recvFile.open(filePath, std::ios::binary | std::ios::trunc);
                if (!session->recvFile.is_open()) {
                    // 本片数据仍在 socket 里，继续循环会把数据当下一个头解析。
                    LOG_ERR("%s failed to open file %s — closing session",
                        statusPrefix.c_str(), filePath.c_str());
                    session->active.store(false);
                    break;
                }
                session->currentFilePath = filePath;
                LOG_INF("%s receiving: %s", statusPrefix.c_str(), filePath.c_str());
            }

            // 按剩余字节数算片大小：空文件时 % 会空等分片，越界时 % 的偏移对不上会写坏文件。
            uint64_t remain = (header.fileSize > offset) ? (header.fileSize - offset) : 0;
            uint32_t dataSize = (remain > header.chunkSize) ? header.chunkSize : (uint32_t)remain;

            if (dataSize > 0) {
                if (session->chunkBuf.size() < dataSize) {
                    session->chunkBuf.resize(dataSize);
                }
                int r = recvAll(sock, session->chunkBuf.data(), dataSize);
                if (r <= 0) {
                    if (r == 0) {
                        LOG_INF("%s client disconnected while receiving data (chunk %u/%u)",
                            statusPrefix.c_str(), header.currentChunk, header.chunkCount);
                    } else {
                        LOG_ERR("%s recv data failed", statusPrefix.c_str());
                    }
                    if (session->recvFile.is_open()) session->recvFile.close();
                    session->active.store(false);
                    break;
                }

                session->recvFile.write(session->chunkBuf.data(), r);
                if (!session->recvFile.good()) {
                    // 磁盘满/IO 错误时不中止会把损坏文件当成功传完
                    LOG_ERR("%s file write failed (disk full?) — closing", statusPrefix.c_str());
                    session->recvFile.close();
                    session->active.store(false);
                    break;
                }
                session->transSize += r;

                // 每增加 1% 打印一次进度，避免逐分片刷屏
                if (header.fileSize > 0) {
                    int pct = (int)(session->transSize * 100 / header.fileSize);
                    if (pct != lastLoggedPct) {
                        auto now = std::chrono::steady_clock::now();
                        double sec = std::chrono::duration<double>(now - lastSpeedTime).count();
                        double mbps = 0.0;
                        if (sec > 0.001) {
                            mbps = (double)(session->transSize - lastSpeedBytes)
                                / sec / (1024.0 * 1024.0);
                        }
                        LOG_INF("%s receiving: %d%% (%llu/%llu bytes, %.1f MB/s)",
                            statusPrefix.c_str(), pct,
                            (unsigned long long)session->transSize,
                            (unsigned long long)header.fileSize, mbps);
                        lastLoggedPct = pct;
                        lastSpeedTime = now;
                        lastSpeedBytes = session->transSize;
                    }
                }
            }

            if (m_progressCallback && session->transSize > 0) {
                int pct = header.fileSize > 0
                    ? (int)(session->transSize * 100 / header.fileSize) : 100;
                if (pct != lastProgressPct || session->transSize == header.fileSize) {
                    lastProgressPct = pct;
                    m_progressCallback(session->transSize, header.fileSize,
                        statusPrefix + "Receiving...", FT_EVT_NONE);
                }
            }
            break;
        }
        case CMD_COMPLETE: {
            LOG_INF("%s TransferEngine complete: %llu bytes",
                statusPrefix.c_str(), (unsigned long long)header.transSize);

            if (session->recvFile.is_open()) {
                session->recvFile.close();
            }

            if (m_progressCallback) {
                // "|" 后带真实落盘路径：同名文件会带序号，上层自己拼的路径不一定存在
                std::string status = statusPrefix + "Transfer complete!";
                if (!session->currentFilePath.empty()) {
                    status += "|" + session->currentFilePath;
                }
                m_progressCallback(header.fileSize, header.fileSize, status, FT_EVT_TRANSFER_COMPLETE);
            }
            session->currentFilePath.clear();
            break;
        }
        case CMD_CANCEL: {
            LOG_INF("%s TransferEngine cancelled", statusPrefix.c_str());

            if (session->recvFile.is_open()) {
                session->recvFile.close();
            }

            // 记录半截文件，等待对端 CMD_DELETE 精确删除
            struct stat st { };
            if (!session->currentFilePath.empty() &&
                stat(session->currentFilePath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                std::lock_guard<std::mutex> lock(m_partialMutex);
                m_cancelledPartials[session->pendingFileName] = session->currentFilePath;
            }

            if (m_progressCallback) {
                // 上报已接收的字节数：传 0 会让 UI 进度条回退到起点
                m_progressCallback(session->transSize, header.fileSize,
                    statusPrefix + "Transfer cancelled", FT_EVT_TRANSFER_CANCELLED);
            }
            break;
        }
        case CMD_DELETE: {
            // 与 CMD_REQUEST 相同的文件名校验与读取
            if (header.fileNameLen == 0 || header.fileNameLen > MAX_FILE_NAME_LEN) {
                LOG_ERR("%s invalid fileNameLen=%u (max %u) — closing",
                    statusPrefix.c_str(), header.fileNameLen, MAX_FILE_NAME_LEN);
                session->active.store(false);
                break;
            }
            std::string fileName(header.fileNameLen, '\0');
            int r = recvAll(sock, &fileName[0], header.fileNameLen);
            if (r <= 0) {
                LOG_ERR("%s recv filename timeout — protocol desync, closing", statusPrefix.c_str());
                session->active.store(false);
                break;
            }
            std::string safeName = sanitizeFileName(fileName);

            // 只删 CMD_CANCEL 时记录过的路径，原始文件名找不到记录则忽略
            std::string partial;
            {
                std::lock_guard<std::mutex> lock(m_partialMutex);
                auto it = m_cancelledPartials.find(safeName);
                if (it != m_cancelledPartials.end()) {
                    partial = it->second;
                    m_cancelledPartials.erase(it);
                }
            }
            struct stat st { };
            if (partial.empty() || stat(partial.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
                LOG_WRN("%s delete request: no partial file recorded for \"%s\"",
                    statusPrefix.c_str(), safeName.c_str());
                break;
            }
            if (remove(partial.c_str()) != 0) {
                LOG_ERR("%s remove(%s) failed: %s",
                    statusPrefix.c_str(), partial.c_str(), strerror(errno));
                break;
            }
            LOG_INF("%s deleted partial file: %s (%llu bytes)",
                statusPrefix.c_str(), partial.c_str(), (unsigned long long)st.st_size);
            if (m_progressCallback) {
                m_progressCallback(0, 0, statusPrefix + "Partial deleted: " + safeName,
                    FT_EVT_NONE);
            }
            break;
        }
        case CMD_RESPONSE:
            // 服务端不应收到 CMD_RESPONSE：协议违规
            LOG_WRN("%s unexpected CMD_RESPONSE (server does not request files)", statusPrefix.c_str());
            break;
        default:
            LOG_WRN("%s unknown command 0x%04x", statusPrefix.c_str(), header.cmd);
            break;
        }
    }

    LOG_INF("%s clientHandler EXIT — cleaning up fd=%d (active=%d, running=%d)",
        statusPrefix.c_str(), sock, (int)session->active.load(), (int)m_serverRunning.load());

    if (session->recvFile.is_open()) {
        session->recvFile.close();
    }
    close(sock);
    m_sessionMgr.removeSession(sock);

    if (m_progressCallback) {
        m_progressCallback(0, 0, "Client disconnected: " + clientIp + ":" + std::to_string(clientPort),
            FT_EVT_SESSION_CLOSED);
    }
}

int TransferEngine::sendHeader(int sock, const FileHeader& header)
{
    // 线上传输用网络序；header 本身始终保持主机序，方便日志与比较
    FileHeader net = header;
    headerToNet(net);
    return sendHeader(sock, &net, sizeof(net));
}

int TransferEngine::sendHeader(int sock, const void* data, size_t len)
{
    std::lock_guard<std::mutex> lock(m_sendMutex);
    if (sendAll(sock, data, len) < 0) {
        LOG_ERR("sendAll() failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int TransferEngine::recvHeader(int sock, FileHeader& header)
{
    int ret = recvAll(sock, &header, sizeof(header));
    if (ret > 0) {
        headerToHost(header);
        return 0;
    }
    return -1;
}

int TransferEngine::sendSliceData(int sock, const std::string& filePath, uint64_t fileSize,
    const std::atomic<bool>& runFlag, std::shared_ptr<std::atomic<bool>> cancelFlag)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERR("ifstream::open() failed: %s", filePath.c_str());
        return -1;
    }

    uint32_t chunkSize = MAX_CHUNK_SIZE;
    // 空文件也发一个分片
    uint32_t chunkCount = (fileSize == 0) ? 1
        : (uint32_t)((fileSize + chunkSize - 1) / chunkSize);
    uint64_t totalSent = 0;
    int lastPct = -1;
    auto lastSpeedTime = std::chrono::steady_clock::now();
    uint64_t lastSpeedBytes = 0;
    bool cancelled = false;
    int ret = 0;
    std::vector<char> buffer;
    if (fileSize > 0) {
        buffer.resize(chunkSize);  // 跨分片复用，避免每片分配并零初始化 256KB
    }

    for (uint32_t i = 0; i < chunkCount && runFlag.load(); ++i) {
        // 取消在分片边界生效：向对端发 CMD_CANCEL，由其停止接收并回调取消事件
        if (cancelFlag && cancelFlag->exchange(false)) {
            cancelled = true;
            break;
        }

        FileHeader header{ };
        fillHeader(header, CMD_DATA);
        header.fileSize = fileSize;
        header.chunkSize = chunkSize;
        header.chunkCount = chunkCount;
        header.currentChunk = i;
        header.transSize = totalSent;

        // 按剩余字节算本片大小（与服务端一致）；空文件只发头不读数据
        std::streamsize bytesRead = 0;
        if (fileSize > 0) {
            uint64_t remain = fileSize - (uint64_t)i * chunkSize;
            uint32_t dataSize = (remain > chunkSize) ? chunkSize : (uint32_t)remain;
            file.read(buffer.data(), dataSize);
            bytesRead = file.gcount();
        }

        // 头与数据须在同一临界区发出：否则并发命令插到中间会让服务端把命令头当文件数据读入。
        FileHeader netHeader = header;
        headerToNet(netHeader);
        {
            std::lock_guard<std::mutex> lock(m_sendMutex);
            if (sendAll(sock, &netHeader, sizeof(netHeader)) < 0
                || (bytesRead > 0 && sendAll(sock, buffer.data(), bytesRead) < 0)) {
                LOG_ERR("sendAll() failed at chunk %u/%u", i, chunkCount);
                ret = -3;
                break;
            }
        }

        totalSent += bytesRead;

        if (m_progressCallback) {
            int pct = fileSize > 0 ? (int)(totalSent * 100 / fileSize) : 100;
            if (pct != lastPct) {
                auto now = std::chrono::steady_clock::now();
                double sec = std::chrono::duration<double>(now - lastSpeedTime).count();
                double mbps = sec > 0.001
                    ? (double)(totalSent - lastSpeedBytes) / sec / (1024.0 * 1024.0) : 0.0;
                LOG_INF("sendSliceData: %d%% (%llu/%llu bytes, %.1f MB/s)",
                    pct, (unsigned long long)totalSent, (unsigned long long)fileSize, mbps);
                m_progressCallback(totalSent, fileSize, "Sending...", FT_EVT_NONE);
                lastPct = pct;
                lastSpeedTime = now;
                lastSpeedBytes = totalSent;
            }
        }
    }

    file.close();

    if (cancelled) {
        FileHeader cancel{ };
        fillHeader(cancel, CMD_CANCEL);
        cancel.fileSize = fileSize;
        cancel.transSize = totalSent;
        cancel.event = FT_EVT_TRANSFER_CANCELLED;
        sendHeader(sock, cancel);  // 尽力而为：对端可能已不在线
        if (m_progressCallback) {
            m_progressCallback(totalSent, fileSize, "Send cancelled", FT_EVT_TRANSFER_CANCELLED);
        }
        LOG_INF("sendSliceData: cancelled at %llu/%llu bytes",
            (unsigned long long)totalSent, (unsigned long long)fileSize);
        return -2;
    }
    if (ret < 0) return ret;  // 发送失败：不发 COMPLETE，避免对端把半截文件当成功

    FileHeader complete{ };
    fillHeader(complete, CMD_COMPLETE);
    complete.fileSize = fileSize;
    complete.transSize = totalSent;
    complete.event = FT_EVT_TRANSFER_COMPLETE;
    if (sendHeader(sock, complete) < 0) {
        LOG_ERR("sendHeader(COMPLETE) failed: %s", strerror(errno));
    }

    if (m_progressCallback) {
        m_progressCallback(totalSent, fileSize, "Send complete!", FT_EVT_TRANSFER_COMPLETE);
    }

    LOG_INF("TransferEngine sent complete: %llu bytes", (unsigned long long)totalSent);
    return 0;
}

int TransferEngine::postLocalFile(const std::string& filePath)
{
    if (!m_connected.load()) {
        LOG_ERR("not connected");
        return -1;
    }
    auto cancelFlag = beginTransfer();

    struct stat st {};
    if (stat(filePath.c_str(), &st) != 0) {
        LOG_ERR("stat() failed: %s", strerror(errno));
        return -2;
    }

    uint64_t fileSize = st.st_size;

    size_t pos = filePath.find_last_of("/\\");
    std::string fileName = (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
    LOG_INF("postLocalFile: [%s] size=%llu bytes", fileName.c_str(), (unsigned long long)fileSize);

    // header + filename 须持锁按序发出：并发数据插入会令服务端按 64 字节头 + fileNameLen 解析错位。
    FileHeader header{ };
    fillHeader(header, CMD_REQUEST);
    header.fileNameLen = (uint32_t)fileName.size();
    header.fileSize = fileSize;
    header.chunkSize = MAX_CHUNK_SIZE;
    header.chunkCount = (uint32_t)((fileSize + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE);

    FileHeader netHeader = header;
    headerToNet(netHeader);
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (sendAll(m_clientSock, &netHeader, sizeof(netHeader)) < 0) {
            LOG_ERR("sendAll(header) failed: %s", strerror(errno));
            return -5;
        }
        if (sendAll(m_clientSock, fileName.c_str(), fileName.size()) < 0) {
            LOG_ERR("sendAll(filename) failed: %s", strerror(errno));
            return -6;
        }
    }

    FileHeader response{ };
    if (recvHeader(m_clientSock, response) < 0 || response.cmd != CMD_RESPONSE) {
        LOG_ERR("recvHeader() failed: no response from server");
        return -3;
    }

    if (m_progressCallback) {
        m_progressCallback(0, fileSize, "Transfer accepted", FT_EVT_NONE);
    }

    return sendSliceData(m_clientSock, filePath, fileSize, m_connected, cancelFlag);
}

int TransferEngine::requestFile(const std::string& ip, unsigned short port, const std::string& fileName)
{
    if (fileName.empty()) {
        LOG_ERR("requestFile: empty fileName");
        return -1;
    }
    auto cancelFlag = beginTransfer();

    int ret = connectToServer(ip, port);
    if (ret < 0) {
        return ret;
    }

    // 拉取请求：event=FT_EVT_REQUEST_PULL 标识模式，fileSize=0 表示本端不带数据
    FileHeader header{ };
    fillHeader(header, CMD_REQUEST);
    header.event = FT_EVT_REQUEST_PULL;
    header.fileNameLen = (uint32_t)fileName.size();

    FileHeader netHeader = header;
    headerToNet(netHeader);
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (sendAll(m_clientSock, &netHeader, sizeof(netHeader)) < 0) {
            LOG_ERR("sendAll(header) failed: %s", strerror(errno));
            return -2;
        }
        if (sendAll(m_clientSock, fileName.c_str(), fileName.size()) < 0) {
            LOG_ERR("sendAll(filename) failed: %s", strerror(errno));
            return -3;
        }
    }

    FileHeader response{ };
    if (recvHeader(m_clientSock, response) < 0 || response.cmd != CMD_RESPONSE) {
        LOG_ERR("recvHeader() failed: no response from server");
        return -4;
    }
    // RESP_ACCEPT 表示接受（fileSize 可为 0 的空文件）；未带标记的旧端沿用 fileSize>0 语义。
    if (response.event != RESP_ACCEPT && response.fileSize == 0) {
        LOG_ERR("pull request rejected by server: %s (not found or not a regular file)",
            fileName.c_str());
        return -5;
    }

    if (m_progressCallback) {
        m_progressCallback(0, response.fileSize, "Pulling: " + fileName, FT_EVT_RECEIVE_START);
    }

    return receivePullData(m_clientSock, fileName, response.fileSize, cancelFlag);
}

// 拉取接收循环：分片语义与服务端 CMD_DATA 一致（顺序校验/写盘检查/1% 日志）
int TransferEngine::receivePullData(int sock, const std::string& fileName, uint64_t fileSize,
    std::shared_ptr<std::atomic<bool>> cancelFlag)
{
    std::string filePath = uniqueSavePath(savePath(), fileName);

    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        LOG_ERR("receivePullData: failed to open file %s", filePath.c_str());
        return -1;
    }
    LOG_INF("receivePullData: saving to %s", filePath.c_str());

    uint64_t received = 0;
    int lastPct = -1;
    int lastProgressPct = -1;
    auto lastSpeedTime = std::chrono::steady_clock::now();
    uint64_t lastSpeedBytes = 0;
    std::vector<char> buf;
    bool sawComplete = false;

    while (m_connected.load()) {
        // 本端取消拉取：停止落盘，连接保持，由上层决定是否断开
        if (cancelFlag && cancelFlag->exchange(false)) {
            out.close();
            if (m_progressCallback) {
                m_progressCallback(received, fileSize, "Pull cancelled", FT_EVT_TRANSFER_CANCELLED);
            }
            LOG_INF("receivePullData: cancelled at %llu/%llu bytes",
                (unsigned long long)received, (unsigned long long)fileSize);
            return -8;
        }

        FileHeader header{ };
        int ret = recvAll(sock, &header, sizeof(header));
        if (ret <= 0) {
            LOG_WRN("receivePullData: recv header failed (ret=%d) — aborting", ret);
            out.close();
            return -2;
        }
        headerToHost(header);

        if (memcmp(header.magic, kProtocolMagic, sizeof(kProtocolMagic)) != 0) {
            LOG_ERR("receivePullData: invalid magic 0x%08x — aborting",
                *reinterpret_cast<uint32_t*>(header.magic));
            out.close();
            return -3;
        }

        if (header.cmd == CMD_COMPLETE) {
            sawComplete = true;
            break;
        }
        if (header.cmd == CMD_CANCEL) {
            // 对端取消了它正在代发的数据
            out.close();
            if (m_progressCallback) {
                m_progressCallback(received, fileSize, "Pull cancelled by peer",
                    FT_EVT_TRANSFER_CANCELLED);
            }
            LOG_INF("receivePullData: cancelled by peer at %llu/%llu bytes",
                (unsigned long long)received, (unsigned long long)fileSize);
            return -8;
        }
        if (header.cmd != CMD_DATA) {
            LOG_ERR("receivePullData: unexpected cmd %u — aborting", header.cmd);
            out.close();
            return -3;
        }
        if (header.chunkSize == 0 || header.chunkSize > MAX_CHUNK_SIZE
            || header.currentChunk >= header.chunkCount
            || header.fileSize != fileSize) {
            LOG_ERR("receivePullData: invalid chunk meta: chunk %u/%u size=%u fileSize=%llu",
                header.currentChunk, header.chunkCount, header.chunkSize,
                (unsigned long long)header.fileSize);
            out.close();
            return -4;
        }

        uint64_t offset = (uint64_t)header.currentChunk * header.chunkSize;
        if (offset != received) {
            LOG_ERR("receivePullData: chunk out of order: chunk %u offset=%llu but received %llu",
                header.currentChunk, (unsigned long long)offset, (unsigned long long)received);
            out.close();
            return -5;
        }

        uint64_t remain = (fileSize > offset) ? (fileSize - offset) : 0;
        uint32_t dataSize = (remain > header.chunkSize) ? header.chunkSize : (uint32_t)remain;
        if (dataSize == 0) {
            continue;
        }

        if (buf.size() < dataSize) {
            buf.resize(dataSize);
        }
        int r = recvAll(sock, buf.data(), dataSize);
        if (r <= 0) {
            LOG_ERR("receivePullData: recv data failed (ret=%d)", r);
            out.close();
            return -6;
        }
        out.write(buf.data(), r);
        if (!out.good()) {
            LOG_ERR("receivePullData: file write failed (disk full?) — aborting");
            out.close();
            return -7;
        }
        received += r;

        int pct = fileSize > 0 ? (int)(received * 100 / fileSize) : 100;
        if (m_progressCallback && (pct != lastProgressPct || received == fileSize)) {
            lastProgressPct = pct;
            m_progressCallback(received, fileSize, "Pulling: " + fileName, FT_EVT_NONE);
        }
        if (pct != lastPct) {
            auto now = std::chrono::steady_clock::now();
            double sec = std::chrono::duration<double>(now - lastSpeedTime).count();
            double mbps = sec > 0.001
                ? (double)(received - lastSpeedBytes) / sec / (1024.0 * 1024.0) : 0.0;
            LOG_INF("receivePullData: %d%% (%llu/%llu bytes, %.1f MB/s)",
                pct, (unsigned long long)received, (unsigned long long)fileSize, mbps);
            lastPct = pct;
            lastSpeedTime = now;
            lastSpeedBytes = received;
        }
    }

    out.close();

    // disconnect() 令 m_connected=false 也会退出循环，此时不能把半截文件当成功。
    if (!sawComplete || received != fileSize) {
        LOG_ERR("receivePullData: incomplete (%llu/%llu bytes, complete=%d) — removing partial %s",
            (unsigned long long)received, (unsigned long long)fileSize,
            (int)sawComplete, filePath.c_str());
        remove(filePath.c_str());
        return -9;
    }

    if (m_progressCallback) {
        m_progressCallback(fileSize, fileSize, "Pull complete!|" + filePath, FT_EVT_TRANSFER_COMPLETE);
    }
    LOG_INF("receivePullData: complete: %llu bytes -> %s",
        (unsigned long long)received, filePath.c_str());
    return 0;
}

