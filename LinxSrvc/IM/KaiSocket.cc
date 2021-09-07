#include "KaiSocket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <thread>

static bool g_thdref = false;

KaiSocket::KaiSocket(unsigned short srvport)
{
    new (this)KaiSocket(nullptr, srvport); // placement new
}

KaiSocket::KaiSocket(const char* srvip, unsigned short servport)
{
    if (srvip != nullptr) {
        m_network.IP = srvip;
    }
    m_network.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_network.socket < 0) {
        std::cerr
            << "Creating socket (" << (errno != 0 ? strerror(errno) : std::to_string(m_network.socket)) << ")."
            << std::endl;
        return;
    }
    m_network.PORT = servport;
}

int KaiSocket::start()
{
    int listen_socket = m_network.socket;
    unsigned short servport = m_network.PORT;

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons((uint16_t)servport);
    if (::bind(listen_socket, reinterpret_cast<struct sockaddr*>(&local), sizeof(local)) < 0) {
        std::cerr
            << "Binding sockaddr_in (" << (errno != 0 ? strerror(errno) : std::to_string(listen_socket)) << ")."
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

    struct sockaddr_in listenAddr;
    socklen_t listenLen = static_cast<socklen_t>(sizeof(listenAddr));
    getsockname(listen_socket, reinterpret_cast<struct sockaddr*>(&listenAddr), &listenLen);
    std::cout << "localhost listening [" << inet_ntoa(listenAddr.sin_addr) << ":" << servport << "]." << std::endl;

    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peerAddr;
    socklen_t peerLen = static_cast<socklen_t>(sizeof(peerAddr));
    m_isClient = false;

    while (1) {
        struct sockaddr_in sin;
        socklen_t len = static_cast<socklen_t>(sizeof(sin));
        int rcv_sock = m_network.socket = ::accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
        if (rcv_sock < 0) {
            std::cerr
                << "Socket accept (" << (errno != 0 ? strerror(errno) : std::to_string(rcv_sock)) << ")."
                << std::endl;
            return -3;
        };
        {
            std::mutex mtxlck{};
            std::lock_guard<std::mutex> lock(mtxlck);
            bool set = true;
            time_t t{};
            time(&t);
            struct tm* lt = localtime(&t);
            g_threadNo_++;
            m_network.run_ = true;
            setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
            getpeername(rcv_sock, reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);
            m_network.IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
            m_network.PORT = ntohs(peerAddr.sin_port);
            fprintf(stdout, "accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n",
                g_threadNo_,
                m_network.IP.c_str(), m_network.PORT,
                lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

            Header head{ 0, 0, m_network.flag.ssid = setSsid(m_network, rcv_sock), 0 };
            ::send(rcv_sock, (char*)&head, sizeof(Header), 0);
            m_networks.emplace_back(m_network);

            std::cout << "socket monitor: " << rcv_sock << "; waiting for massage." << std::endl;
            for (std::vector<int(*)(KaiSocket*)>::iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
                std::thread(&KaiSocket::runCallback, this, this, (*it)).detach();
            }
            wait(100);
        }
    }
    return 0;
}

int KaiSocket::connect()
{
    const char* ipaddr = m_network.IP.c_str();
    sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
    srvaddr.sin_port = htons(m_network.PORT);
    std::cout << "------ client v0.1: " << ipaddr << " ------" << std::endl;
    bool bReuseaddr = false;
    setsockopt(m_network.socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
    int tries = 0;
    while (::connect(m_network.socket, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == (-1)) {
        if (tries < 100) {
            wait(100);
            tries++;
        } else {
            std::cerr << "Trying to connect (" << "tries=" << tries << ", "
                << (errno != 0 ? strerror(errno) : "No error") << ")." << std::endl;
            handleNotify(m_network.socket);
            return -1;
        }
    }
    m_network.run_ = true;
    for (std::vector<int(*)(KaiSocket*)>::iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
        std::thread(&KaiSocket::runCallback, this, this, (*it)).detach();
    }
    while (running()) {
        wait(100);
    }
    return 0;
}

int KaiSocket::send(const char* data, int len)
{
    if (data == nullptr || len == 0)
        return 0;
    int head = sizeof(Header);
    size_t left = len + head;
    char mesg[left];
    memset(mesg, 0, left);
    memcpy(mesg, &m_network.flag, head);
    memcpy(mesg + head, data, len);
    const char* ptr = mesg;
    while (left > 0) {
        ssize_t wrote;
        if ((wrote = write(m_network.socket, ptr, left)) <= 0) {
            if (wrote < 0 && errno == EINTR)
                wrote = 0; /* call write() again */
            else {
                handleNotify(m_network.socket);
                return -1; /* error */
            }
        }
        left -= wrote;
        ptr += wrote;
    }
    return (len + head - left);
}

int KaiSocket::broadcast(const char* data, int len)
{
    if (m_networks.size() == 0 || m_networks.begin() == m_networks.end())
        return -1;
    int bytes = 0;
    for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
        int head = sizeof(Header);
        int size = head + len;
        char mesg[size];
        memset(mesg, 0, size);
        memcpy(mesg, &it->flag, head);
        memcpy(mesg + head, data, len);
        bytes = ::send(it->socket, mesg, size, 0);
        if (bytes <= 0) {
            handleNotify(it->socket);
            continue;
        }
        wait(1);
    }
    return bytes;
}

int KaiSocket::broker(char* data, int len)
{
    int res = ::recv(m_network.socket, data, len, 0);
    if (res < 0) {
        handleNotify(m_network.socket);
        return -1;
    }
    if (data[0] == 'K' && data[1] == 'a' && data[2] == 'i')
        return 0;
    if (res == 0 || m_networks.size() == 1)
        return res;
    if (m_networks.size() == 0 || m_networks.begin() == m_networks.end())
        return -2;
    for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
        if (strcmp(it->flag.mqid, m_network.flag.mqid) == 0 && it->socket != m_network.socket) {
            if (::send(it->socket, data, res, 0) <= 0) {
                handleNotify(it->socket);
                it = m_networks.begin();
                continue;
            }
            wait(1);
        }
    }
    return res;
}
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-undefined-compare"
#pragma GCC diagnostic ignored "-Wtautological-pointer-compare"
#pragma GCC diagnostic ignored "-Wundefined-bool-conversion"
#endif
int KaiSocket::recv(char* buff, int len)
{
    if (buff == nullptr || len == 0)
        return 0;
    int size = sizeof(Header);
    char header[size];
    memset(header, 0, size);
    if (m_networks.size() == 0) {
        handleNotify(m_network.socket);
        return -1;
    }
    m_isClient = true;
    int res = ::recv(m_network.socket, header, size, 0);
    if (res < 0) {
        handleNotify(m_network.socket);
        return -2;
    }
    // get ssid set to 'm_network', also repeat to server as a mark for search clients
    if (res == size) {
        std::mutex mtxlck = {};
        std::lock_guard<std::mutex> lock(mtxlck);
        unsigned long long ssid = reinterpret_cast<Header*>(header)->ssid;
        if (ssid != 0) {
            m_network.flag.ssid = ntohl(ssid);
            memcpy(m_network.flag.mqid, reinterpret_cast<Header*>(header)->mqid, 32);
            for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
                if (&(*it) == nullptr || &m_network == nullptr)
                    continue;
                if (verifySsid(it->socket, m_network.flag.ssid)) {
                    it->flag.ssid = setSsid(m_network, it->socket);
                    strcpy(it->flag.mqid, m_network.flag.mqid);
                }
                wait(1);
            }
        } else {
            size = 0;
        }
    } else {
        if (strncmp(header, "Kai", 3) == 0)
            return 0; // heartbeat ignore
    }
    memcpy(buff, header, res);
    int err = ::recv(m_network.socket, buff + size, len, 0);
    if (err <= 0) {
        handleNotify(m_network.socket);
        err = -3;
    }
    Message mesg = *reinterpret_cast<Message*>(buff);
    int tag = mesg.head.etag;
    int ret = 0;
    switch (tag) {
    case 1:
        ret = produce(mesg);
        break;
    case 2:
        ret = consume(mesg);
        break;
    default:
        break;
    }
    if (tag != 0) {
        if (ret == 0)
            strcpy(mesg.data.stat, "SUCCESS");
        else if (ret == -1)
            strcpy(mesg.data.stat, "NULLPTR");
        else
            strcpy(mesg.data.stat, "FAILURE");
        this->send((char*)&mesg, sizeof(Message));
    }
    return (err + res);
}

bool KaiSocket::running()
{
    if (!this)
        return false;
    return m_network.run_;
}
#ifdef __GNUC__
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

void KaiSocket::registerCallback(int(*func)(KaiSocket*))
{
    m_callbacks.clear();
    appendCallback(func);
}

void KaiSocket::appendCallback(int(*func)(KaiSocket*))
{
    if (std::find(m_callbacks.begin(), m_callbacks.end(), func) == m_callbacks.end()) {
        m_callbacks.emplace_back(func);
    }
}

void KaiSocket::appendCallback(std::function<int(KaiSocket*)> func)
{
    int(*hook)(KaiSocket*) { /*&func*/ }; // TODO covert func to unset functional
    appendCallback(hook);
}

void KaiSocket::setResponseHandle(void(*func)(char*, int), char* data, int& size)
{
    auto hook = [func, data, &size](KaiSocket* sock)-> int {
        if (sock == nullptr)
            return -1;
        int len = sock->recv(data, size);
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

void KaiSocket::setRequestHandle(void(*func)(char*, int), char* data, int& size)
{
    auto hook = [func, data, &size](KaiSocket* sock)-> int {
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

void KaiSocket::handleNotify(int socket)
{
    if (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT || errno == EWOULDBLOCK)
        return;
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
    for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
        //FIXME: signal SIGSEGV, Segmentation fault.
        if (socket < 0 || it->socket < 0 || m_networks.size() == 0)
            return;
        if (it->socket == socket && it->run_) {
            {
                std::vector<Network>::iterator iter = it;
                it->run_ = false;
                std::cerr
                    << "### " << (m_isClient ? "Server" : "Client")
                    << "(" << it->IP << ":" << it->PORT << ") socket [" << it->socket << "] lost."
                    << std::endl;
                if (m_networks.size() > 0) {
                    close(it->socket);
                    if (m_networks.size() == 1) {
                        m_networks.clear();
                        break;
                    }
                    if (m_networks.end() == m_networks.erase(iter)) return;
                    it = m_networks.begin();
                } else return;
            }
        }
        wait(1);
    }
}

KaiSocket::~KaiSocket()
{
    close(m_network.socket);
}

KaiSocket& KaiSocket::GetInstance()
{
    static KaiSocket socket;
    return socket;
}

void KaiSocket::runCallback(KaiSocket* sock, int (*func)(KaiSocket*))
{
    if (m_isClient && !g_thdref) {
        // heartBeat
        std::thread(
            [](Network& m_network, KaiSocket* sock) {
                while (sock->running()) {
                    if (::send(m_network.socket, "Kai", 3, 0) <= 0) {
                        std::cerr << "Heartbeat to " << m_network.IP << ":"
                            << m_network.PORT << " arrests." << std::endl;
                        sock->handleNotify(m_network.socket);
                        break;
                    }
                    KaiSocket::wait(30000); // frequency
                }
            }, std::ref(m_network), sock).detach();
            g_thdref = !g_thdref;
    }
    if (func != nullptr) {
        while (sock->running()) {
            int ret = func(sock);
            if (ret != 0) {
                std::cerr << "Callback status = " << ret << std::endl;
            }
        }
    }
}

unsigned long long KaiSocket::setSsid(const Network& m_network, int socket)
{
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
    unsigned int ip = 0;
    const char* s = reinterpret_cast<char*>((unsigned char**)&m_network.IP);
    unsigned char t = 0;
    while (1) {
        if (*s != '\0' && *s != '.') {
            t = (unsigned char)(t * 10 + *s - '0');
        } else {
            ip = (ip << 8) + t;
            if (*s == '\0')
                break;
            t = 0;
        }
        s++;
    };
    return (m_network.PORT << 16 | socket << 8 | ip);
}

bool KaiSocket::verifySsid(int key, unsigned long long ssid)
{
    return ((int)((ssid >> 8) & 0x00ff) == key);
}

int KaiSocket::ProduceClient(const std::string& body, ...)
{
    Message msg = {};
    memset(&msg, 0, sizeof(Message));
    m_network.flag.etag = msg.head.etag = Producer;
    size_t size = body.size();
    memcpy(msg.data.body, body.c_str(), size > 256 ? 256 : size);
    int res = this->send((char*)&msg, sizeof(Message));
    if (res < 0)
        return -1;
    return res;
}

int KaiSocket::ConsumeClient()
{
    Message msg = {};
    memset(&msg, 0, sizeof(Message));
    m_network.flag.etag = msg.head.etag = Consumer;
    return this->send((char*)&msg, sizeof(Message));
}

int KaiSocket::produce(const Message& msg)
{
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
    if (msgque == nullptr)
        return -1;
    size_t size = msgque->size();
    if (msg.head.ssid != 0) {
        msgque->emplace_back(new Message(msg));
    }
    return static_cast<int>(msgque->size() - size - 1);
}

int KaiSocket::consume(Message& msg)
{
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
    if (msgque == nullptr)
        return -1;
    if (msgque->empty())
        return -2;
    size_t size = msgque->size();
    Message* mesg = msgque->front();
    memcpy(&msg.head, &mesg->head, sizeof(Header));
    memcpy(&msg.data, &mesg->data, sizeof(Message::Payload));
    msgque->pop_front();
    if (mesg != nullptr) {
        delete mesg;
    }
    return static_cast<int>(size - msgque->size() - 1);
}
