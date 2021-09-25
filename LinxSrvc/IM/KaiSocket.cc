#include "KaiSocket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <cmath>

static bool g_thrStat = false;
static unsigned int g_maxRetryTimes = 100;
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
    if (srvip != nullptr) {
        m_network.IP = srvip;
    }
    m_network.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_network.socket < 0) {
        std::cerr
            << "Generating socket (" << (errno != 0 ? strerror(errno) : std::to_string(m_network.socket)) << ")."
            << std::endl;
        return;
    }
    m_network.PORT = srvport;
}

int KaiSocket::start()
{
    int listen_socket = m_network.socket;
    unsigned short srvport = m_network.PORT;

    struct sockaddr_in local { };
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons((uint16_t)srvport);
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
        int rcv_sock = m_network.socket = ::accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
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

            Header head{ 0, 0, m_network.flag.ssid = setSsid(m_network, rcv_sock), 0 };
            ::send(rcv_sock, (char*)&head, HEAD_SIZE, 0);
            m_networks.emplace_back(m_network);

            std::cout << "socket monitor: " << rcv_sock << "; waiting for massage." << std::endl;
            for (auto& callback : m_callbacks) {
                std::thread(&KaiSocket::runCallback, this, this, callback).detach();
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
    int tries = 0;
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
    for (auto& callback : m_callbacks) {
        std::thread(&KaiSocket::runCallback, this, this, callback).detach();
    }
    while (running()) {
        wait(g_wait100ms);
    }
    return 0;
}

int KaiSocket::send(const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
        return 0;
    size_t head = HEAD_SIZE;
    size_t left = len + head;
    char* msg = new char[left];
    memset(msg, 0, left);
    memcpy(msg, &m_network.flag, head);
    memcpy(msg + head, data, len);
    while (left > 0) {
        ssize_t wrote = 0;
        if ((wrote = write(m_network.socket, msg + wrote, left)) <= 0) {
            if (wrote < 0 && errno == EINTR)
                wrote = 0; /* call write() again */
            else {
                handleNotify(m_network);
                delete[] msg;
                return -1; /* error */
            }
        }
        left -= wrote;
    }
    delete[] msg;
    return static_cast<int>(len + head - left);
}

int KaiSocket::broadcast(const char* data, int len)
{
    if (m_networks.empty() || m_networks.begin() == m_networks.end())
        return -1;
    ssize_t bytes = 0;
    for (auto& network : m_networks) {
        int head = HEAD_SIZE;
        int size = head + len;
        char msg[size];
        memset(msg, 0, size);
        memcpy(msg, &network.flag, head);
        memcpy(msg + head, data, len);
        bytes = ::send(network.socket, msg, size, 0);
        if (bytes <= 0) {
            handleNotify(network);
            continue;
        }
        wait(1);
    }
    return static_cast<int>(bytes);
}

int recvhook(KaiSocket* kai)
{
    KaiSocket::Message msg = {};
    const size_t Size = sizeof(KaiSocket::Message);
    memset(&msg, 0, Size);
    size_t len = kai->recv((char*)(&msg), Size);
    std::cout
        << __FUNCTION__ << ": msg from " << (msg.head.etag == KaiSocket::Producer ? "Producer" : "Consumer")
        << ", MQ topic: '" << msg.head.topic << "', message: '" << msg.data.body << "'"
        << ", len = " << len
        << std::endl;
    return static_cast<int>(len);
}

int KaiSocket::broker()
{
    registerCallback(recvhook);
    return this->start();
}

#if (defined __GNUC__ && __APPLE__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-undefined-compare"
#pragma GCC diagnostic ignored "-Wtautological-pointer-compare"
#pragma GCC diagnostic ignored "-Wundefined-bool-conversion"
#endif

int KaiSocket::recv(char* buff, int size)
{
    if (buff == nullptr || size == 0)
        return 0;
    int len = HEAD_SIZE;
    char header[HEAD_SIZE];
    memset(header, 0, len);
    if (m_networks.empty()) {
        handleNotify(m_network);
        return -1;
    }
    ssize_t res = ::recv(m_network.socket, header, len, 0);
    if (res < 0) {
        handleNotify(m_network);
        return -2;
    }
    // get ssid set to 'm_network', also repeat to server as a mark for search clients
    if (res == len) {
        std::mutex mtxLck = {};
        std::lock_guard<std::mutex> lock(mtxLck);
        unsigned long long ssid = reinterpret_cast<Header*>(header)->ssid;
        if (ssid != 0) {
            m_network.flag.ssid = ntohl((long)ssid);
            memcpy(m_network.flag.topic, reinterpret_cast<Header*>(header)->topic, 32);
            for (auto& network : m_networks) {
                if (&network == nullptr || &m_network == nullptr)
                    continue;
                if (verifySsid(network.socket, m_network.flag.ssid)) {
                    network.flag.ssid = setSsid(m_network, m_network.socket);
                    strcpy(network.flag.topic, m_network.flag.topic);
                }
                wait(1);
            }
        } else {
            len = 0;
        }
    } else {
        if (strncmp(header, "Kai", 3) == 0)
            return 0; // heartbeat ignore
    }
    memcpy(buff, header, res);
    ssize_t err = ::recv(m_network.socket, buff + len, size, 0);
    if (err <= 0) {
        handleNotify(m_network);
        err = -3;
    }
    Message msg = *reinterpret_cast<Message*>(buff);
    int tag = msg.head.etag;
    int ret = 0;
    switch (tag) {
    case 1:
        ret = produce(msg);
        break;
    case 2:
        ret = consume(msg);
        break;
    default:
        break;
    }
    if (tag != 0) {
        if (ret == 0)
            strcpy(msg.data.stat, "SUCCESS");
        else if (ret == -1)
            strcpy(msg.data.stat, "NULLPTR");
        else
            strcpy(msg.data.stat, "FAILURE");
        this->send((uint8_t*)&msg, sizeof(Message));
    }
    return static_cast<int>(err + res);
}

bool KaiSocket::running()
{
    std::lock_guard<std::mutex> lock(m_lock);
    if (!this)
        return false;
    return m_network.run_;
}

#if (defined __GNUC__ && __APPLE__ )
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

void KaiSocket::setResponseHandle(void(*func)(uint8_t*, size_t), uint8_t* data, size_t& size)
{
    auto hook = [func, data, &size](KaiSocket* sock)-> size_t {
        if (sock == nullptr)
            return -1;
        int len = sock->recv(reinterpret_cast<char*>(data), (int)size);
        if (len == size) {
            func(data, size);
        } else if (len != 0) {
            std::cerr << "Received size/len (" << len << ", " << size << ") not match." << std::endl;
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
        int len = sock->send(data, size);
        if (len != size) {
            std::cerr << "Sent size/len (" << len << ", " << size << ") not match." << std::endl;
        }
        size = len;
        return size;
    };
    appendCallback(hook);
}

void KaiSocket::handleNotify(Network& network)
{
    if (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT || errno == EWOULDBLOCK)
        return;
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto it = m_networks.begin(); it != m_networks.end(); ++it) {
        if (it->socket < 0 || it->socket < 0 || m_networks.empty())
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

KaiSocket& KaiSocket::GetInstance()
{
    static KaiSocket socket;
    return socket;
}

void KaiSocket::runCallback(KaiSocket* sock, KAISOCKHOOK func)
{
    if (m_network.flag.etag != Producer) {
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
        if (func == nullptr) {
            continue;
        }
        int len = func(sock);
        if (len < 0) {
            std::cerr << "callback size = " << len << std::endl;
            break;
        }
    }
}

unsigned long long KaiSocket::setSsid(const Network& network, int socket)
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
    return (network.PORT << 16 | socket << 8 | ip);
}

bool KaiSocket::verifySsid(int key, unsigned long long ssid)
{
    std::lock_guard<std::mutex> lock(m_lock);
    return ((int)((ssid >> 8) & 0x00ff) == key);
}

int KaiSocket::produce(const Message& msg)
{
    std::lock_guard<std::mutex> lock(m_lock);
    if (m_msgQue == nullptr)
        return -1;
    size_t size = m_msgQue->size();
    if (msg.head.ssid != 0) {
        m_msgQue->emplace_back(new(msg)Message());
    }
    return static_cast<int>(m_msgQue->size() - size - 1);
}

int KaiSocket::consume(Message& msg)
{
    if (m_msgQue == nullptr)
        return -1;
    if (m_msgQue->empty())
        return -2;

    std::lock_guard<std::mutex> lock(m_lock);
    size_t size = m_msgQue->size();
    Message* msgQ = m_msgQue->front();
    memcpy(&msg.head, &msgQ->head, sizeof(Header));
    memcpy(&msg.data, &msgQ->data, sizeof(Message::Payload));

    m_msgQue->pop_front();
    delete msgQ;
    return static_cast<int>(size - m_msgQue->size() - 1);
}

void KaiSocket::SetTopic(const std::string& topic, int tag)
{
    m_network.flag.etag = tag;
    size_t size = topic.size();
    if (size > 32) {
        std::cerr << __FUNCTION__ << ": topic length " << size << " out of bounds." << std::endl;
        size = 32;
    }
    memcpy(m_network.flag.topic, topic.c_str(), size);
}

ssize_t KaiSocket::Subscriber(const std::string& message)
{
    Message msg = {};
    const size_t Size = sizeof(Message);
    memset(&msg, 0, Size);
    msg.head.etag = Consumer;
    // parse message divide to topic/etc...
    std::string topic = "message.sub()...";
    SetTopic(topic, msg.head.etag);
    this->connect();
    ssize_t len = this->send((uint8_t*)&msg, Size);
    if (len < 0) {
        std::cerr << __FUNCTION__ << ": " << strerror(errno) << std::endl;
        return len;
    }
    ssize_t size = 0;
    do {
        memset(&msg, 0, Size);
        len = ::recv(m_network.socket, &msg, Size, 0);
        if (len < 0) {
            std::cerr << __FUNCTION__ << ": " << strerror(errno) << std::endl;
            handleNotify(m_network);
            return -1;
        }
        char* kai = (char*)(&msg);
        if (kai[0] == 'K' && kai[1] == 'a' && kai[2] == 'i')
            return 0;
        size_t more = msg.head.size - Size;
        char body[more];
        if (more > 0) {
            len = ::recv(m_network.socket, body, more, 0);
            if (len < 0) {
                std::cerr << __FUNCTION__ << ": " << strerror(errno) << std::endl;
                handleNotify(m_network);
                return -1;
            }
        }
        std::cout
            << __FUNCTION__ << " as: " << (msg.head.etag == Producer ? "Producer" : "Consumer")
            << ", MQ topic: '" << msg.head.topic << "', message: '" << more << "'"
            << std::endl;
        size += len;
    } while (len > 0);
    return size;
#ifdef TEST_CNSM
    if (m_networks.size() == 0 || m_networks.begin() == m_networks.end())
        return -1;
    for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
        if (strcmp(it->flag.mqid, m_network.flag.mqid) == 0 && it->socket != m_network.socket) {
            Message msg;
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
}

int KaiSocket::Publisher(const std::string& topic, const std::string& payload, ...)
{
    size_t size = payload.size();
    if (topic.empty() || size == 0) {
        std::cerr << __FUNCTION__ << ": topic/payload is null" << std::endl;
    } else {
        this->m_network.run_ = false;
        g_maxRetryTimes = 0;
        m_callbacks.clear();
    }
    const int maxLen = 256 + 8;
    Message msg = {};
    memset(&msg, 0, sizeof(Message));
    size_t msgLen = sizeof msg + size;
    msg.head.size = msgLen;
    msg.head.etag = Producer;
    SetTopic(topic, msg.head.etag);
    if (this->connect() != 0) {
        std::cerr << __FUNCTION__ << ": connect failed" << std::endl;
        return -1;
    }
    auto* message = new uint8_t(msgLen);
    memcpy(message, &msg, msgLen);
    memcpy(message + sizeof(msg), payload.c_str(), size > maxLen ? maxLen : size);
    this->produce(msg);
    int len = this->send(message, msgLen);
    if (len <= 0) {
        this->consume(msg);
    }
    finish();
    return len;
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
