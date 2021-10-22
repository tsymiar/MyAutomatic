#include "KaiSocket.h"

#ifdef _WIN32
#include <Ws2tcpip.h>
#include <Windows.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <cmath>

#ifdef _WIN32
#define close(s) {closesocket(s);WSACleanup();}
typedef int socklen_t;
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#define usleep(u) Sleep((u)/1000)
#define write(a,b,c) ::send(a,(char*)(b),c,0)
#else
#define WSACleanup()
#endif

static bool g_thrStat = false;
static unsigned int g_maxRetryTimes = 100;
char KaiSocket::G_KaiRole[][0xa] = { "NONE", "PRODUCER", "CONSUMER", "SERVER", "BROKER", "CLIENT", "PUBLISH", "SUBSCRIBE" };
namespace {
    const unsigned int g_wait100ms = 100;
    const size_t HEAD_SIZE = sizeof(KaiSocket::Header);
}

KaiSocket::KaiSocket(unsigned short lstnprt)
{
    new (this)KaiSocket(nullptr, lstnprt); // placement new
}

KaiSocket::KaiSocket(const char* srvip, unsigned short srvport)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(srvport, &wsaData) == SOCKET_ERROR) {
        std::cerr << "WSAStartup failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
        return;
    }
#endif // _WIN32
    if (srvip != nullptr) {
        m_network.IP = srvip;
    }
    m_network.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_network.socket < 0) {
        std::cerr
            << "Generating socket (" << (errno != 0 ? strerror(errno) : std::to_string(m_network.socket)) << ")."
            << std::endl;
        WSACleanup();
        return;
    }
    m_network.PORT = srvport;
}

KaiSocket& KaiSocket::GetInstance()
{
    static KaiSocket socket;
    return socket;
}

int KaiSocket::start()
{
    SOCKET listen_socket = m_network.socket;
    unsigned short srvport = m_network.PORT;

    struct sockaddr_in local { };
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(srvport);
    if (::bind(listen_socket, reinterpret_cast<struct sockaddr*>(&local), sizeof(local)) < 0) {
        std::cerr
            << "Binding socket address (" << (errno != 0 ? strerror(errno) : std::to_string(listen_socket)) << ")."
            << std::endl;
        close(listen_socket);
        return -1;
    }

    const int backlog = 50;
    if (listen(listen_socket, backlog) < 0) {
        std::cerr
            << "Socket listen (" << (errno != 0 ? strerror(errno) : std::to_string(listen_socket)) << ")."
            << std::endl;
        close(listen_socket);
        return -2;
    }

    struct sockaddr_in lstnaddr { };
    auto listenLen = static_cast<socklen_t>(sizeof(lstnaddr));
    getsockname(listen_socket, reinterpret_cast<struct sockaddr*>(&lstnaddr), &listenLen);
    std::cout << "localhost listening [" << inet_ntoa(lstnaddr.sin_addr) << ":" << srvport << "]." << std::endl;
    m_isClient = false;

    while (true) {
        struct sockaddr_in sin { };
        auto len = static_cast<socklen_t>(sizeof(sin));
        SOCKET rcv_sock = m_network.socket = ::accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
        if (rcv_sock < 0) {
            std::cerr
                << "Socket accept (" << (errno != 0 ? strerror(errno) : std::to_string(rcv_sock)) << ")."
                << std::endl;
            return -3;
        }
        {
            std::mutex mtxLck{};
            std::lock_guard<std::mutex> lock(mtxLck);
            bool set = true;
            time_t t{};
            time(&t);
            struct tm* lt = localtime(&t);
            char ipaddr[INET_ADDRSTRLEN];
            struct sockaddr_in peeraddr { };
            auto peerLen = static_cast<socklen_t>(sizeof(peeraddr));
            m_threadNo_++;
            setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
            getpeername(rcv_sock, reinterpret_cast<struct sockaddr*>(&peeraddr), &peerLen);
            m_network.IP = inet_ntop(AF_INET, &peeraddr.sin_addr, ipaddr, sizeof(ipaddr));
            m_network.PORT = ntohs(peeraddr.sin_port);
            fprintf(stdout, "accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n",
                m_threadNo_,
                m_network.IP.c_str(), m_network.PORT,
                lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

            Header head{ 0, 0, m_network.flag.ssid = setSsid(m_network, rcv_sock), {0} };
            ::send(rcv_sock, (char*)&head, HEAD_SIZE, 0);
            m_networks.emplace_back(m_network);

            std::cout << "socket monitor: " << rcv_sock << "; waiting for massage." << std::endl;
            for (auto& callback : m_callbacks) {
                if (callback == nullptr)
                    continue;
                try {
                    std::thread(&KaiSocket::runCallback, this, this, callback).detach();
                } catch (...) {
                    std::cerr << __FUNCTION__ << ": catch (...) exception" << std::endl;
                }
            }
            wait(g_wait100ms);
        }
    }
}

int KaiSocket::connect()
{
    m_isClient = true;
    sockaddr_in srvaddr{};
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(m_network.PORT);
    const char* ipaddr = m_network.IP.c_str();
    srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
    std::cout << "------ Client v0.1 " << ipaddr << ":" << m_network.PORT << " ------" << std::endl;
    bool addrreuse = false;
    setsockopt(m_network.socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&addrreuse, sizeof(bool));
    unsigned int tries = 0;
    while (::connect(m_network.socket, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == (-1)) {
        if (tries < g_maxRetryTimes) {
            wait(g_wait100ms * (int)pow(2, tries));
            tries++;
        } else {
            std::cerr << "Retrying to connect (" << "tries=" << tries << ", "
                << (errno != 0 ? strerror(errno) : "No error") << ")." << std::endl;
            handleNotify(m_network);
            return -1;
        }
    }
    m_networks.emplace_back(m_network);
    for (auto& callback : m_callbacks) {
        std::thread(&KaiSocket::runCallback, this, this, callback).detach();
    }
    while (running()) {
        wait(g_wait100ms);
    }
    return 0;
}

int proxyhook(KaiSocket* kai)
{
    if (kai == nullptr) {
        std::cout << "KaiSocket instance is NULL" << std::endl;
        return -1;
    }
    KaiSocket::Message msg = {};
    const size_t Size = sizeof(KaiSocket::Message);
    memset(&msg, 0, Size);
    int len = kai->recv(reinterpret_cast<uint8_t*>(&msg), Size);
    if (len > 0) {
        std::cout
            << __FUNCTION__ << ": message from " << KaiSocket::G_KaiRole[msg.head.etag]
            << ", MQ topic: '" << msg.head.topic
            << "', len = " << len
            << ", while " << msg.data.stat
            << std::endl;
    }
    return len;
}

int KaiSocket::broker()
{
    registerCallback(proxyhook);
    return this->start();
}

#if (defined __GNUC__ && __APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-undefined-compare"
#pragma GCC diagnostic ignored "-Wtautological-pointer-compare"
#pragma GCC diagnostic ignored "-Wundefined-bool-conversion"
#endif

ssize_t KaiSocket::recv(uint8_t* buff, size_t size)
{
    if (buff == nullptr || size == 0)
        return -1;
    const size_t len = HEAD_SIZE;
    KaiSocket::Header header = {};
    memset(&header, 0, len);
    if (m_networks.empty()) {
        handleNotify(m_network);
        return -1;
    }
    ssize_t res = ::recv(m_network.socket, reinterpret_cast<char*>(&header), len, 0);
    if (res < 0) {
        handleNotify(m_network);
        return -2;
    }
    if (strncmp(reinterpret_cast<char*>(&header), "Kai", 3) == 0)
        return 0; // heartbeat ignore
    if (res != len) {
        std::cout << __FUNCTION__ << ": got len " << res << ", size = " << len << std::endl;
    }
    std::mutex mtxLck = {};
    std::lock_guard<std::mutex> lock(mtxLck);
    static uint64_t subSsid;
    // get ssid set to 'm_network', also repeat to server as a mark for search clients
    unsigned long long ssid = header.ssid;
    for (auto& network : m_networks) {
        if (!m_network.run_ || m_network.socket == 0)
            continue;
        // select to set consume network
        if (verifySsid(network.socket, ssid)) {
            subSsid = ssid;
            network.flag.etag = header.etag;
            memcpy(network.flag.topic, header.topic, sizeof(Header::topic));
        }
        wait(1);
    }
    m_network.flag.etag = header.etag;
    m_network.flag.size = header.size;
    memcpy(m_network.flag.topic, header.topic, sizeof(Header::topic));
    memcpy(buff, &header, len);
    size_t total = m_network.flag.size;
    auto* message = new(std::nothrow) uint8_t[total];
    if (message == nullptr) {
        std::cerr << __FUNCTION__ << ": message malloc failed!" << std::endl;
        return -1;
    }
    ssize_t left = m_network.flag.size - len;
    ssize_t err = -1;
    if (left > 0) {
        err = ::recv(m_network.socket, reinterpret_cast<char*>(message + len), left, 0);
        if (err <= 0) {
            handleNotify(m_network);
            return -3;
        }
    }
    Message msg = *reinterpret_cast<Message*>(buff);
    ssize_t stat = 0;
    switch (msg.head.etag) {
    case PRODUCER:
        stat = produce(msg);
        break;
    case CONSUMER:
        stat = consume(msg);
        break;
    default:
        break;
    }
    if (msg.head.etag >= NONE && msg.head.etag <= SUBSCRIBE) {
        std::cout << __FUNCTION__ << ": " << G_KaiRole[msg.head.etag] << " operated count = "
            << (stat > 0 ? std::to_string(stat) : "0") << std::endl;
    }
    if (msg.head.etag != 0) {
        using namespace std;
        msg.head.ssid = setSsid(m_network);
        memcpy(message, &msg, sizeof(Message));
        for (auto& network : m_networks) {
            if (strcmp(network.flag.topic, m_network.flag.topic) == 0
                && network.flag.ssid == subSsid
                && network.flag.etag == CONSUMER) { // only consume should be sent
                if ((stat = this->sendto(network, message, total)) < 0) {
                    std::cerr << __FUNCTION__ << ": sendto [" << network.socket << "], " << total << " failed." << std::endl;
                    continue;
                }
            }
        }
        if (stat >= 0)
            strcpy(msg.data.stat, "SUCCESS");
        else if (stat == -1)
            strcpy(msg.data.stat, "NULLPTR");
        else
            strcpy(msg.data.stat, "FAILURE");
        memcpy(buff + HEAD_SIZE, msg.data.stat, sizeof(Message::data.stat));
    } else { // set running is to delete m_network
        m_network.run_ = true;
        handleNotify(m_network);
        std::cerr << __FUNCTION__ << ": unsupported role = " << msg.head.etag << std::endl;
        return -4;
    }
    return (err + res);
}

bool KaiSocket::running()
{
    std::lock_guard<std::mutex> lock(m_lock);
    return m_network.run_;
}

#if (defined __GNUC__ && __APPLE__)
#pragma GCC diagnostic pop
#endif

void KaiSocket::wait(unsigned int tms)
{
#ifdef USE_SELECT
    const int THOS = 1000;
    struct timeval delay = {
        .tv_sec = time_t(tms / THOS),
        .tv_usec = (long)(tms % THOS * THOS)
    };
    select(0, NULL, NULL, NULL, &delay);
#else
    usleep(1000 * tms);
#endif
}

ssize_t KaiSocket::sendto(Network network, const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
        return 0;
    int left = (int)len;
    auto* msg = new uint8_t[left];
    memset(msg, 0, left);
    memcpy(msg, data, len);
    while (left > 0) {
        ssize_t wrote = 0;
        if ((wrote = write(network.socket, reinterpret_cast<char*>(msg + wrote), left)) <= 0) {
            if (wrote < 0 && errno == EINTR)
                wrote = 0; /* call write() again */
            else {
                handleNotify(network);
                delete[] msg;
                return -1; /* error */
            }
        }
        left -= wrote;
    }
    delete[] msg;
    return ssize_t(len - left);
}

ssize_t KaiSocket::broadcast(const uint8_t* data, size_t len)
{
    if (m_networks.empty() || m_networks.begin() == m_networks.end())
        return -1;
    ssize_t bytes = 0;
    for (auto& network : m_networks) {
        size_t size = HEAD_SIZE + len;
        auto* message = new(std::nothrow) uint8_t[size];
        if (message == nullptr) {
            std::cerr << __FUNCTION__ << ": message malloc failed!" << std::endl;
            return -1;
        }
        memset(message, 0, size);
        memcpy(message, &network.flag, HEAD_SIZE);
        memcpy(message + HEAD_SIZE, data, len);
        ssize_t stat = sendto(network, message, size);
        if (stat <= 0) {
            handleNotify(network);
            stat = 0;
            continue;
        }
        bytes += stat;
        wait(1);
    }
    return bytes;
}

void KaiSocket::registerCallback(KAISOCKHOOK func)
{
    m_callbacks.clear();
    appendCallback(func);
}

void KaiSocket::appendCallback(KAISOCKHOOK func)
{
    if (std::find(m_callbacks.begin(), m_callbacks.end(), func) == m_callbacks.end()) {
        m_callbacks.emplace_back(func);
    }
}

void KaiSocket::appendCallback(const std::function<int(KaiSocket*)>& func)
{
    int(*hook)(KaiSocket*) { /*&func*/ }; // TODO covert func to unset functional
    appendCallback(hook);
}

void KaiSocket::handleNotify(Network& network)
{
    if (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT || errno == EWOULDBLOCK) {
        std::cerr << __FUNCTION__ << ": " << strerror(errno) << std::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto it = m_networks.begin(); it != m_networks.end(); ++it) {
        if (it->socket < 0 || m_network.socket < 0 || m_networks.empty())
            return;
        if (it->socket == network.socket && it->run_) {
            auto iter = it;
            it->run_ = false;
            std::cerr
                << "### " << (m_isClient ? "Server" : "Client")
                << "(" << it->IP << ":" << it->PORT << ") socket [" << it->socket << "] lost."
                << std::endl;
            close(it->socket);
            if (m_networks.size() == 1) {
                m_networks.clear();
                break;
            }
            if (m_networks.end() == m_networks.erase(iter))
                return;
            it = m_networks.begin();
        }
        wait(1);
    }
}

void KaiSocket::runCallback(KaiSocket* sock, KAISOCKHOOK func)
{
    if (m_network.flag.etag != PRODUCER) {
        sock->m_network.run_ = true;
    }
    if (m_isClient && !g_thrStat) {
        // heartBeat
        std::thread(
            [](Network& network, KaiSocket* kai) {
                while (kai->running()) {
                    if (::send(network.socket, "Kai", 3, 0) <= 0) {
                        std::cerr << "Heartbeat to " << network.IP << ":"
                            << network.PORT << " arrests." << std::endl;
                        kai->handleNotify(network);
                        break;
                    }
                    KaiSocket::wait(3000); // frequency 3s
                }
            }, std::ref(m_network), sock).detach();
            g_thrStat = !g_thrStat;
    }
    while (sock->running()) {
        wait(g_wait100ms);
        if (func == nullptr) {
            continue;
        }
        int len = func(sock);
        if (len <= 0) {
            std::cerr << "callback status = " << len << std::endl;
            break;
        }
    }
}

uint64_t KaiSocket::setSsid(const Network& network, SOCKET socket)
{
    std::lock_guard<std::mutex> lock(m_lock);
    unsigned int ip = 0;
    const char* s = reinterpret_cast<char*>((unsigned char**)&network.IP);
    unsigned char t = 0;
    while (true) {
        if (*s != '\0' && *s != '.') {
            t = (unsigned char)(t * 10 + *s - '0');
        } else {
            ip = (ip << 8) + t;
            if (*s == '\0')
                break;
            t = 0;
        }
        s++;
    }
    if (socket == 0) {
        socket = network.socket;
    }
    return (network.PORT << 16 | socket << 8 | ip);
}

bool KaiSocket::verifySsid(SOCKET key, uint64_t ssid)
{
    std::lock_guard<std::mutex> lock(m_lock);
    return ((int)((ssid >> 8) & 0x00ff) == key);
}

void KaiSocket::setResponseHandle(void(*func)(uint8_t*, size_t), uint8_t* data, size_t& size)
{
    auto hook = [func, data, &size](KaiSocket* sock)-> size_t {
        if (sock == nullptr)
            return -1;
        size_t len = sock->recv(data, size);
        if (len == size) {
            func(data, size);
        } else if (len != 0) {
            std::cerr << "Received size/len (" << len << ", " << size << ") mismatch." << std::endl;
        }
        size = len;
        return size;
    };
    appendCallback(hook);
}

void KaiSocket::setRequestHandle(void(*func)(uint8_t*, size_t), uint8_t* data, size_t& size)
{
    auto hook = [func, data, &size](KaiSocket* sock)-> size_t {
        if (sock == nullptr)
            return -1;
        func(data, size);
        size_t len = sock->send(data, size);
        if (len != size) {
            std::cerr << "Sent size/len (" << len << ", " << size << ") mismatch." << std::endl;
        }
        size = len;
        return size;
    };
    appendCallback(hook);
}

int KaiSocket::produce(const Message& msg)
{
    if (m_msgQue == nullptr) {
        std::cerr << "Message struct queue is null." << std::endl;
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    size_t size = m_msgQue->size();
    if (msg.head.ssid != 0 || msg.head.etag == PRODUCER) {
        // Message* mess = new(msg)Message();
        m_msgQue->emplace_back(&msg);
    }
    return static_cast<int>(m_msgQue->size() - size - 1);
}

int KaiSocket::consume(Message& msg)
{
    if (m_msgQue == nullptr || m_msgQue->empty()) {
        std::cerr << "Message struct queue is null/empty." << std::endl;
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    size_t size = m_msgQue->size();
    const Message* msgQ = m_msgQue->front();
    memcpy(&msg.head, &msgQ->head, sizeof(Header));
    memcpy(&msg.data, &msgQ->data, sizeof(Message::Payload));
    m_msgQue->pop_front();

    return static_cast<int>(size - m_msgQue->size() - 1);
}

void KaiSocket::finish()
{
    m_network.run_ = false;
    usleep(g_wait100ms);
    close(m_network.socket);
    m_callbacks.clear();
    delete m_msgQue;
    m_msgQue = nullptr;
}

ssize_t KaiSocket::send(const uint8_t* data, size_t len)
{
    return broadcast(data, len);
}

void KaiSocket::SetTopic(const std::string& topic, Header& header)
{
    size_t size = topic.size();
    if (size > sizeof(header.topic)) {
        std::cerr << __FUNCTION__ << ": topic length " << size << " out of bounds." << std::endl;
        size = sizeof(header.topic);
    }
    memcpy(header.topic, topic.c_str(), size);
    memcpy(m_network.flag.topic, header.topic, size);
    m_network.flag.etag = header.etag;
}

ssize_t KaiSocket::Subscriber(const std::string& message, RECVCALLBACK callback)
{
    if (this->connect() < 0)
        return -2;
    Message msg = {};
    const size_t Size = sizeof(Message);
    bool flag = false;
    do {
        memset(&msg, 0, Size);
        ssize_t len = ::recv(m_network.socket, reinterpret_cast<char*>(&msg), Size, 0);
        if (len < 0) {
            std::cerr << __FUNCTION__ << ": recv head fail, " << strerror(errno) << std::endl;
            handleNotify(m_network);
            return -3;
        }
        char* kai = (char*)(&msg);
        if (kai[0] == 'K' && kai[1] == 'a' && kai[2] == 'i')
            continue;
        flag = (msg.head.ssid != 0);
        if (msg.head.size == 0) {
            msg.head.size = Size;
            msg.head.etag = CONSUMER;
            if (msg.head.ssid == 0) {
                msg.head.ssid = setSsid(m_network);
            }
            // parse message divide to topic/etc...
            const std::string topic = message; // "message.sub()...";
            SetTopic(topic, msg.head);
            len = sendto(m_network, (uint8_t*)&msg, Size);
            if (len < 0) {
                std::cerr << __FUNCTION__ << ": sendto " << strerror(errno) << std::endl;
                return -4;
            }
            std::cout << __FUNCTION__ << " as " << KaiSocket::G_KaiRole[msg.head.etag]
                << ", MQ topic: '" << msg.head.topic << "'." << std::endl;
        }
        if (msg.head.size > Size) {
            size_t remain = msg.head.size - Size;
            auto* body = new(std::nothrow) uint8_t[remain];
            if (body == nullptr) {
                std::cerr << __FUNCTION__ << ": body malloc failed!" << std::endl;
                return -1;
            }
            len = ::recv(m_network.socket, reinterpret_cast<char*>(body), remain, 0);
            if (len < 0) {
                std::cerr << __FUNCTION__ << ": recv body fail, " << strerror(errno) << std::endl;
                handleNotify(m_network);
                return -5;
            } else {
                memcpy(msg.data.body, body, len);
                if (callback != nullptr) {
                    callback(msg);
                }
                std::cout << __FUNCTION__ << ": message payload = [" << msg.data.stat << "]-[" << msg.data.body << "]" << std::endl;
            }
            delete[] body;
        }
    } while (flag);
#ifdef TEST_CONSUME
    if (m_networks.size() == 0 || m_networks.begin() == m_networks.end())
        return -1;
    for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
        if (strcmp(it->flag.mqid, m_network.flag.mqid) == 0 && it->socket != m_network.socket) {
            Message msg = {};
            if (consume(msg) >= 0) {
                if (::send(it->socket, message.c_str(), message.size(), 0) <= 0) {
                    handleNotify(*it);
                    it = m_networks.begin();
                    continue;
                }
            } else {
                std::cerr << "consume fail, socket: " << it->socket << ", topic: " << msg.head.mqid << "(" << it->flag.mqid << ")" << std::endl;
            }
            wait(1);
            }
        }
#endif
    return 0;
    }

ssize_t KaiSocket::Publisher(const std::string& topic, const std::string& payload, ...)
{
    size_t size = payload.size();
    if (topic.empty() || size == 0) {
        std::cerr << __FUNCTION__ << ": topic/payload is null" << std::endl;
    } else {
        this->m_network.run_ = false;
        g_maxRetryTimes = 0;
        m_callbacks.clear();
    }
    const int maxLen = 256;
    Message msg = {};
    memset(&msg, 0, sizeof(Message));
    size = (size > maxLen ? maxLen : size);
    size_t msgLen = sizeof msg + size;
    msg.head.size = static_cast<unsigned int>(msgLen);
    msg.head.etag = PRODUCER;
    SetTopic(topic, msg.head);
    if (this->connect() != 0) {
        std::cerr << __FUNCTION__ << ": connect failed!" << std::endl;
        return -2;
    }
    auto* message = new(std::nothrow) uint8_t[msgLen];
    if (message == nullptr) {
        std::cerr << __FUNCTION__ << ": message malloc failed!" << std::endl;
        return -1;
    }
    memcpy(message, &msg, sizeof(Message));
    memcpy(message + sizeof(msg), payload.c_str(), size);
    this->produce(msg);
    ssize_t len = this->send(message, msgLen);
    if (len <= 0) {
        this->consume(msg);
    }
    delete[] message;
    finish();
    return len;
}
