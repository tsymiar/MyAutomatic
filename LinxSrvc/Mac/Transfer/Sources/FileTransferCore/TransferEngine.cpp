//
// Created by Shenyrion on 2025.
//

#include "TransferEngine.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <memory>
#include <vector>
#include <map>

// --- Helper: fill FileHeader with common defaults ---
static inline void fillHeader(FileHeader& h, uint16_t cmd) {
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, "FTF\0", 4);
    h.version = 1;
    h.cmd = cmd;
}

// --- 可靠的 send 辅助函数 ---
// send() 不保证一次发送所有数据（只返回实际发送量）。
// sendAll 循环调用 send() 直到全部发送或出错，与 recvAll 对称。
// 返回值: 0 成功，-1 发送失败
static int sendAll(int sock, const void* buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t ret = send(sock, (const char*)buf + total, len - total, 0);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;  // 非阻塞/中断，重试
            }
            return -1;  // 真正的发送错误
        }
        if (ret == 0) {
            return -1;  // send 返回 0 表示连接关闭
        }
        total += (size_t)ret;
    }
    return 0;
}

// --- 可靠的 recv 辅助函数 ---
// 使用 poll() 检测连接状态，以手动读循环替代 MSG_WAITALL，避免 SO_RCVTIMEO + MSG_WAITALL 的兼容性问题
// 返回值: >0 成功（应等于len），0 客户端断开(EOF/POLLHUP)，-1 超时（可重试），-2 其他错误
static int recvAll(int sock, void* buf, size_t len, int timeoutMs)
{
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;

    size_t total = 0;
    while (total < len) {
        int pr = poll(&pfd, 1, timeoutMs);
        if (pr == 0) {
            return -1;  // 超时，无数据
        }
        if (pr < 0) {
            return -2;  // poll 错误
        }
        if (pfd.revents & (POLLHUP | POLLERR)) {
            return 0;   // 客户端断开
        }
        if (!(pfd.revents & POLLIN)) {
            continue;
        }

        ssize_t ret = recv(sock, (char*)buf + total, len - total, 0);
        if (ret == 0) {
            return 0;   // EOF，客户端正常关闭
        }
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;  // 非阻塞模式下无数据，继续 poll
            }
            return -2;  // recv 错误
        }
        total += ret;
    }
    return (int)total;
}

// --- ClientSessionMgr ---

void ClientSessionMgr::addSession(int sock, const std::string& ip, unsigned short port)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unique_ptr<ClientSession> session(new ClientSession());
    session->sock = sock;
    session->clientIp = ip;
    session->clientPort = port;
    session->active = true;
    m_sessions[sock] = std::move(session);
    LOG_INF("Client session added: %s:%d (total: %zu)", ip.c_str(), port, m_sessions.size());
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
        LOG_INF("Client session removed (total: %zu)", m_sessions.size());
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
            LOG_INF("closeAllSockets: closing client %s:%d (fd=%d)",
                pair.second->clientIp.c_str(), pair.second->clientPort, pair.second->sock);
            shutdown(pair.second->sock, SHUT_RDWR);
            close(pair.second->sock);
            pair.second->sock = -1;
        }
    }
}

size_t ClientSessionMgr::size() const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    return m_sessions.size();
}

// --- TransferEngine ---

TransferEngine::TransferEngine()
    : m_serverSock(-1)
    , m_clientSock(-1)
    , m_currentSock(-1)
    , m_serverRunning(false)
    , m_connected(false)
    , m_running(false)
    , m_serverPort(0)
{}

TransferEngine::~TransferEngine()
{
    stopServer();
    disconnect();
}

void TransferEngine::setSavePath(const std::string& path)
{
    m_savePath = path;
}

void TransferEngine::setProgressCallback(ProgressCallback callback)
{
    m_progressCallback = callback;
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

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
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

    // Keepalive to detect broken connections early
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(ip.c_str());
        if (he == nullptr) {
            LOG_ERR("gethostbyname() failed: invalid address %s", ip.c_str());
            close(sock);
            return -2;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // ── Non-blocking connect with timeout via poll() ──
    // Blocking connect() can hang for 75+ seconds on macOS.
    // We switch to O_NONBLOCK → connect() → poll(POLLOUT) → restore.
    int origFlags = fcntl(sock, F_GETFL, 0);
    if (origFlags < 0) {
        LOG_ERR("fcntl(F_GETFL) failed: %s", strerror(errno));
        close(sock);
        return -3;
    }
    fcntl(sock, F_SETFL, origFlags | O_NONBLOCK);

    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        LOG_ERR("connect() to %s:%u immediate fail: %s", ip.c_str(), port, strerror(errno));
        close(sock);
        return -3;
    }

    if (ret < 0) {
        // EINPROGRESS — wait for completion or timeout
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

        // Verify the connection succeeded
        int sockErr = 0;
        socklen_t len = sizeof(sockErr);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErr, &len) < 0 || sockErr != 0) {
            LOG_ERR("connect() to %s:%u failed: %s",
                ip.c_str(), port, sockErr ? strerror(sockErr) : "getsockopt error");
            close(sock);
            return -3;
        }
    }

    // Restore original blocking mode
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
    m_running.store(true);

    m_serverThread = std::thread(&TransferEngine::fileServerProcess, this);
    return 0;
}

void TransferEngine::stopServer()
{
    m_serverRunning.store(false);
    m_running.store(false);

    // 关闭所有客户端 socket，立即中断其阻塞的 recv() 调用
    m_sessionMgr.closeAllSockets();

    if (m_serverSock >= 0) {
        // shutdown 立即中断阻塞的 accept() 调用
        shutdown(m_serverSock, SHUT_RDWR);
        close(m_serverSock);
        m_serverSock = -1;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
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

    m_currentSock.store(m_clientSock);
    m_serverIp = ip;
    m_serverPort = port;
    m_connected.store(true);
    m_running.store(true);

    m_receiveThread = std::thread(&TransferEngine::fileClientProcess, this);
    return 0;
}

void TransferEngine::disconnect()
{
    m_connected.store(false);
    m_running.store(false);

    if (m_clientSock >= 0) {
        LOG_INF("disconnect: closing connection to %s:%d (fd=%d)",
            m_serverIp.c_str(), m_serverPort, (int)m_clientSock);
        close(m_clientSock);
        m_clientSock = -1;
    }

    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }

    LOG_INF("TransferEngine disconnected");
}

void TransferEngine::fileServerProcess()
{
    struct sockaddr_in clientAddr {};
    socklen_t addrLen = sizeof(clientAddr);

    while (m_serverRunning.load()) {
        int clientSock = accept(m_serverSock, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSock < 0) {
            // accept 已由 stopServer() 中的 shutdown() 中断
            if (m_serverRunning.load()) {
                LOG_ERR("accept() failed: %s", strerror(errno));
            }
            break;
        }

        char ipStr[INET_ADDRSTRLEN];
        unsigned short clientPort = ntohs(clientAddr.sin_port);
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
        LOG_INF("TransferEngine client connected from %s:%d", ipStr, clientPort);

        // 为每个客户端创建独立线程处理
        std::thread(&TransferEngine::clientHandler, this, clientSock, std::string(ipStr), clientPort).detach();
    }
}

void TransferEngine::fileClientProcess()
{
    while (m_connected.load() && m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_INF("fileClientProcess: client receive thread exit (connected=%d, running=%d)",
        (int)m_connected.load(), (int)m_running.load());
}

void TransferEngine::clientHandler(int sock, const std::string& clientIp, unsigned short clientPort)
{
    // 添加会话到管理器
    m_sessionMgr.addSession(sock, clientIp, clientPort);
    ClientSession* session = m_sessionMgr.getSession(sock);
    if (!session) {
        close(sock);
        return;
    }

    // 通知上层：客户端已连接
    if (m_progressCallback) {
        m_progressCallback(0, 0, "Client connected from " + clientIp + ":" + std::to_string(clientPort));
    }

    std::string statusPrefix = "[" + clientIp + ":" + std::to_string(clientPort) + "] ";

    while (m_running.load() && session->active.load()) {
        FileHeader header{};
        int ret = recvAll(sock, &header, sizeof(header), RECV_POLL_TIMEOUT_MS);
        if (ret <= 0) {
            if (ret == -1) {
                // 超时，回到 while 检查 m_running 是否被 stopServer() 置为 false
                continue;
            }
            if (ret == 0) {
                LOG_INF("%s client disconnected (EOF/POLLHUP)", statusPrefix.c_str());
            } else {
                LOG_WRN("%s recv header failed", statusPrefix.c_str());
            }
            break;
        }

        // 验证魔数
        if (memcmp(header.magic, "FTF\0", 4) != 0) {
            LOG_ERR("%s invalid magic number: 0x%08x, client disconnected", statusPrefix.c_str(), *(uint32_t*)header.magic);
            break;
        }

        LOG_INF("%s Received command: %d", statusPrefix.c_str(), header.cmd);
        switch (header.cmd) {
        case CMD_REQUEST: {
            // 读取文件名
            std::string fileName(header.fileNameLen, '\0');
            int r = recvAll(sock, &fileName[0], header.fileNameLen, RECV_POLL_TIMEOUT_MS);
            if (r <= 0) {
                if (r == 0) {
                    LOG_INF("%s client disconnected while reading filename", statusPrefix.c_str());
                } else {
                    LOG_ERR("%s recv filename timeout — protocol desync, closing", statusPrefix.c_str());
                }
                // Protocol is now out of sync; exit while loop by clearing active flag.
                session->active.store(false);
                break;
            }

            LOG_INF("%s TransferEngine incoming request: %s (%llu bytes)",
                statusPrefix.c_str(), fileName.c_str(), (unsigned long long)header.fileSize);

            session->pendingFileName = fileName;
            session->pendingFileSize = header.fileSize;
            session->transSize = 0;

            // 发送接受响应
            FileHeader response{};
            fillHeader(response, CMD_RESPONSE);
            response.fileSize = header.fileSize;
            sendHeader(sock, response);

            // 通知进度
            if (m_progressCallback) {
                m_progressCallback(0, header.fileSize, statusPrefix + "Receiving: " + fileName);
            }
            break;
        }
        case CMD_DATA: {
            // 新文件开始
            if (header.currentChunk == 0 && session->recvFile.is_open()) {
                session->recvFile.close();
            }

            if (!session->recvFile.is_open()) {
                std::string filePath = m_savePath.empty() ? "./" : m_savePath;
                if (!filePath.empty() && filePath.back() != '/' && filePath.back() != '\\') {
                    filePath += "/";
                }
                // 添加客户端前缀避免文件名冲突
                filePath += clientIp + "_" + std::to_string(clientPort) + "_" +
                    (session->pendingFileName.empty() ? "received_file" : session->pendingFileName);

                session->recvFile.open(filePath, std::ios::binary | std::ios::trunc);
                if (!session->recvFile.is_open()) {
                    LOG_ERR("%s failed to open file %s", statusPrefix.c_str(), filePath.c_str());
                    break;
                }
                LOG_INF("%s receiving: %s", statusPrefix.c_str(), filePath.c_str());
            }

            // 接收数据
            uint32_t dataSize = header.chunkSize;
            if (header.currentChunk == header.chunkCount - 1) {
                dataSize = (uint32_t)(header.fileSize % header.chunkSize);
                if (dataSize == 0) dataSize = header.chunkSize;
            }

            std::vector<char> buffer(dataSize);
            int r = recvAll(sock, buffer.data(), dataSize, RECV_POLL_TIMEOUT_MS);
            if (r <= 0) {
                if (r == 0) {
                    LOG_INF("%s client disconnected while receiving data (chunk %u/%u)",
                        statusPrefix.c_str(), header.currentChunk, header.chunkCount);
                } else {
                    LOG_ERR("%s recv data failed", statusPrefix.c_str());
                }
                if (session->recvFile.is_open()) session->recvFile.close();
                break;
            }

            session->recvFile.write(buffer.data(), r);
            session->transSize += r;

            // 进度回调
            if (m_progressCallback && session->transSize > 0) {
                m_progressCallback(session->transSize, header.fileSize,
                    statusPrefix + "Receiving...");
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
                m_progressCallback(header.fileSize, header.fileSize,
                    statusPrefix + "Transfer complete!");
            }
            break;
        }
        case CMD_CANCEL: {
            LOG_INF("%s TransferEngine cancelled", statusPrefix.c_str());

            if (session->recvFile.is_open()) {
                session->recvFile.close();
            }

            if (m_progressCallback) {
                m_progressCallback(0, header.fileSize, statusPrefix + "Transfer cancelled");
            }
            break;
        }
        default:
            LOG_WRN("%s unknown command 0x%04x", statusPrefix.c_str(), header.cmd);
            break;
        }
    }

    // 清理会话
    if (session->recvFile.is_open()) {
        session->recvFile.close();
    }
    close(sock);
    m_sessionMgr.removeSession(sock);

    if (m_progressCallback) {
        m_progressCallback(0, 0, "Client disconnected: " + clientIp + ":" + std::to_string(clientPort));
    }
}

int TransferEngine::sendHeader(int sock, const FileHeader& header)
{
    return sendHeader(sock, &header, sizeof(header));
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
    int ret = recvAll(sock, &header, sizeof(header), RECV_RESP_TIMEOUT_MS);
    return (ret > 0) ? 0 : -1;
}

int TransferEngine::sendFileData(int sock, const std::string& filePath, uint64_t fileSize)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERR("ifstream::open() failed: %s", filePath.c_str());
        return -1;
    }

    uint32_t chunkSize = MAX_CHUNK_SIZE;
    uint32_t chunkCount = (uint32_t)((fileSize + chunkSize - 1) / chunkSize);
    uint64_t totalSent = 0;
    int lastPct = -1;  // 去重进度回调

    for (uint32_t i = 0; i < chunkCount && m_connected.load(); ++i) {
        // 发送数据头
        FileHeader header{};
        fillHeader(header, CMD_DATA);
        header.fileSize = fileSize;
        header.chunkSize = chunkSize;
        header.chunkCount = chunkCount;
        header.currentChunk = i;
        header.transSize = totalSent;

        if (sendHeader(sock, header) < 0) break;

        // 读取并发送数据
        uint32_t dataSize = chunkSize;
        if (i == chunkCount - 1) {
            dataSize = (uint32_t)(fileSize % chunkSize);
            if (dataSize == 0) dataSize = chunkSize;
        }

        std::vector<char> buffer(dataSize);
        file.read(buffer.data(), dataSize);
        std::streamsize bytesRead = file.gcount();

        {
            std::lock_guard<std::mutex> lock(m_sendMutex);
            if (sendAll(sock, buffer.data(), bytesRead) < 0) {
                LOG_ERR("sendAll() failed at chunk %u/%u", i, chunkCount);
                break;
            }
        }

        totalSent += bytesRead;

        // 进度回调（每 10% 或最后一块）
        if (m_progressCallback) {
            int pct = (int)(totalSent * 100 / fileSize);
            if (pct != lastPct) {
                m_progressCallback(totalSent, fileSize, "Sending...");
                lastPct = pct;
            }
        }
    }

    file.close();

    // 发送完成消息
    FileHeader complete{};
    fillHeader(complete, CMD_COMPLETE);
    complete.fileSize = fileSize;
    complete.transSize = totalSent;
    sendHeader(sock, complete);

    if (m_progressCallback) {
        m_progressCallback(totalSent, fileSize, "Send complete!");
    }

    LOG_INF("TransferEngine sent complete: %llu bytes", (unsigned long long)totalSent);
    return 0;
}

int TransferEngine::sendLocalFile(const std::string& filePath)
{
    if (!m_connected.load()) {
        LOG_ERR("not connected");
        return -1;
    }

    struct stat st {};
    if (stat(filePath.c_str(), &st) != 0) {
        LOG_ERR("stat() failed: %s", strerror(errno));
        return -2;
    }

    uint64_t fileSize = st.st_size;

    // 提取文件名
    size_t pos = filePath.find_last_of("/\\");
    std::string fileName = (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
    LOG_INF("sendLocalFile: [%s] size=%llu bytes", fileName.c_str(), (unsigned long long)fileSize);

    // 发送请求头
    FileHeader header{};
    fillHeader(header, CMD_REQUEST);
    header.fileNameLen = (uint32_t)fileName.size();
    header.fileSize = fileSize;
    header.chunkSize = MAX_CHUNK_SIZE;
    header.chunkCount = (uint32_t)((fileSize + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE);

    // ── 原子发送 header + filename ──
    // 两次 sendHeader 若分开调用，mutex 在中间释放，服务器可能
    // 在收到 header 后立刻读取 filename 却发现数据未到 → 协议错位。
    // 这里把 header 和 filename 合并为一次 sendAll，彻底消除竞态。
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (sendAll(m_clientSock, &header, sizeof(header)) < 0) {
            LOG_ERR("sendAll(header) failed: %s", strerror(errno));
            return -5;
        }
        if (sendAll(m_clientSock, fileName.c_str(), fileName.size()) < 0) {
            LOG_ERR("sendAll(filename) failed: %s", strerror(errno));
            return -5;
        }
    }

    // 等待响应
    FileHeader response{};
    if (recvHeader(m_clientSock, response) < 0 || response.cmd != CMD_RESPONSE) {
        LOG_ERR("recvHeader() failed: no response from server");
        return -3;
    }

    if (m_progressCallback) {
        m_progressCallback(0, fileSize, "Transfer accepted");
    }

    // 发送文件数据
    return sendFileData(m_clientSock, filePath, fileSize);
}

int TransferEngine::requestFile(const std::string& ip, unsigned short port, const std::string& fileName)
{
    int ret = connectToServer(ip, port);
    if (ret < 0) {
        return ret;
    }

    // 发送请求
    FileHeader header{};
    fillHeader(header, CMD_REQUEST);
    header.fileNameLen = (uint32_t)fileName.size();

    std::lock_guard<std::mutex> lock(m_sendMutex);
    sendAll(m_clientSock, &header, sizeof(header));
    sendAll(m_clientSock, fileName.c_str(), fileName.size());

    return 0;
}
