#include "KaiSocket.h"

#ifdef _WIN32
#include <Ws2tcpip.h>
#include <Windows.h>
#include <signal.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef USE_EPOLL
#include <sys/epoll.h>
#include <sys/resource.h>
#include <fcntl.h>
#endif
#include <unistd.h>
#include <csignal>
#endif
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <cmath>

#ifdef _WIN32
#define close(s) {closesocket(s);WSACleanup();}
typedef int socklen_t;
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
// #define usleep(u) Sleep((u)/1000)
#define write(x,y,z) ::send(x,(char*)(y),z,0)
#define signal(_1,_2) {}
#else
#define WSACleanup()
#ifdef USE_EPOLL
const int g_epollMax = 1024;
#endif
#endif
const int g_hText = 0x12345678;
static bool g_thrStat = false;
static unsigned int g_maxTimes = 100;
volatile unsigned int g_thrNo_ = 0;
std::deque<const KaiSocket::Message*>* KaiSocket::m_msgQue = new(std::nothrow)std::deque<const KaiSocket::Message*>();
char KaiSocket::G_KaiRole[][0xe] = { "NONE", "PRODUCER", "CONSUMER", "SERVER", "BROKER", "CLIENT", "PUBLISH", "SUBSCRIBE", "FILE_CONTENT" };
namespace {
    const unsigned int WAIT100ms = 100;
    const size_t HEAD_SIZE = sizeof(KaiSocket::Header);
}

void signalCatch(int signo)
{
    if (signo == SIGSEGV)
        return;
    std::cout << "Caught signal: " << signo << std::endl;
}

bool KaiSocket::isLittleEndian()
{
    union {
        int i;
        char c;
    } v;
    v.i = 1;
    return (!(v.c == 1));
}

int KaiSocket::Initialize(const char* ip, unsigned short port)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(port, &wsaData) == SOCKET_ERROR) {
        std::cerr << "WSAStartup fails with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }
#else
    signal(SIGPIPE, signalCatch);
    signal(SIGSEGV, signalCatch);
#endif // _WIN32
    if (ip != nullptr) {
        m_network.IP = ip;
    }
#ifdef USE_EPOLL
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = g_epollMax;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
        perror("setrlimit");
        return -2;
    }
#endif
    m_network.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_network.socket < 0) {
        std::cerr
            << "Generating socket (" << (errno != 0 ? strerror(errno) : std::to_string(m_network.socket)) << ")."
            << std::endl;
        WSACleanup();
        return -3;
    }
    m_network.PORT = port;
    return 0;
}

#ifdef FULLY_COMPILE
KaiSocket::KaiSocket(unsigned short lsn_prt)
{
    new (this)KaiSocket(nullptr, lsn_prt); // placement new
}

KaiSocket::KaiSocket(const char* ip, unsigned short port)
{
    (void)Initialize(ip, port);
}
#else
int KaiSocket::Initialize(unsigned short port)
{
    return Initialize(nullptr, port);
}

#endif
KaiSocket& KaiSocket::GetInstance()
{
    static KaiSocket socket;
    return socket;
}

int KaiSocket::start()
{
    struct sockaddr_in local { };
    unsigned short port = m_network.PORT;
    local.sin_port = htons(port);
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    SOCKET listen_socket = m_network.socket;
#ifdef USE_EPOLL
    const char reuse = 0;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(char));
    if (fcntl(listen_socket, F_SETFL, fcntl(listen_socket, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -6;
    }
#endif
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

    struct sockaddr_in lsn { };
    auto listenLen = static_cast<socklen_t>(sizeof(lsn));
    getsockname(listen_socket, reinterpret_cast<struct sockaddr*>(&lsn), &listenLen);
    std::cout << "localhost listening [" << inet_ntoa(lsn.sin_addr) << ":" << port << "]." << std::endl;
#ifdef USE_EPOLL
    struct epoll_event ev_pll, events[g_epollMax];;
    int poll_desc = epoll_create(g_epollMax);
    ev_pll.events = EPOLLIN | EPOLLET;
    ev_pll.data.fd = listen_socket;
    int ctl = epoll_ctl(poll_desc, EPOLL_CTL_ADD, listen_socket, &ev_pll);
    if (ctl < 0) {
        perror("epoll_ctl");
        return -5;
    }
#endif
    m_network.client = false;
    while (true) {
#ifdef USE_EPOLL
        int actives = epoll_wait(poll_desc, events, g_epollMax, -1);
        if (actives == -1) {
            perror("epoll_wait");
        }
        for (int i = 0; i < actives; i++) {
            if ((!events[i].events) & EPOLLIN) {
                continue;
            }
            if (events[i].data.fd == listen_socket) {
#else
        {
#endif
                struct sockaddr_in client { };
                auto len = static_cast<socklen_t>(sizeof(client));
                SOCKET conn_sock = m_network.socket = ::accept(listen_socket, reinterpret_cast<struct sockaddr*>(&client), &len);
                if ((int)conn_sock < 0) {
                    std::cout
                        << "Socket accept (" << (errno != 0 ? strerror(errno) : std::to_string(conn_sock)) << ")."
                        << std::endl;
                    return -3;
                }
#ifdef USE_EPOLL
                ev_pll.events = EPOLLIN | EPOLLET;
                ev_pll.data.fd = conn_sock;
                char* addr = inet_ntoa(client.sin_addr);
                if (epoll_ctl(poll_desc, EPOLL_CTL_ADD, conn_sock, &ev_pll) < 0) {
                    std::cerr
                        << "Failed to add " << addr << "socket (" << conn_sock << ") to epoll: " << strerror(errno)
                        << std::endl;
                    return -4;
                }
#endif
            {
                std::mutex mtxLck{};
                std::lock_guard<std::mutex> lock(mtxLck);
                time_t t{};
                time(&t);
                struct tm* lt = localtime(&t);
                char ipaddr[INET_ADDRSTRLEN];
                struct sockaddr_in peer { };
                auto peerLen = static_cast<socklen_t>(sizeof(peer));
                g_thrNo_++;
                bool set = true;
                setsockopt(m_network.socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
                getpeername(m_network.socket, reinterpret_cast<struct sockaddr*>(&peer), &peerLen);
                m_network.IP = inet_ntop(AF_INET, &peer.sin_addr, ipaddr, sizeof(ipaddr));
                m_network.PORT = ntohs(peer.sin_port);
                fprintf(stdout, "accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n",
                    g_thrNo_,
                    m_network.IP.c_str(), m_network.PORT,
                    lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

                Header head{ 0, 0, m_network.flag.ssid = setSsid(m_network), {0} };
                ::send(m_network.socket, (char*)&head, HEAD_SIZE, 0);
                m_networks.emplace_back(m_network);

                std::cout << "socket monitor: " << m_network.socket << "; waiting massage..." << std::endl;
                for (auto& callback : m_callbacks) {
                    if (callback == nullptr)
                        continue;
                    try {
                        std::thread(&KaiSocket::runCallback, this, this, callback).detach();
                    } catch (const std::exception& e) {
                        std::cerr << __FUNCTION__ << ": catch (...) exception: " << e.what() << std::endl;
                    }
                }
                wait(WAIT100ms);
            }
#ifdef USE_EPOLL
            }
#endif
        }
    } // while
}

int KaiSocket::connect()
{
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(m_network.PORT);
    const char* ipaddr = m_network.IP.c_str();
    serv.sin_addr.s_addr = inet_addr(ipaddr);
    m_network.client = true;
    unsigned int tries = 0;
    const char reuse = 0;
    setsockopt(m_network.socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(char));
    std::cout << "------ Client v0.1 connecting to " << ipaddr << ":" << m_network.PORT << " ------" << std::endl;
    while (::connect(m_network.socket, (struct sockaddr*)&serv, sizeof(serv)) == (-1)) {
        if (tries < g_maxTimes) {
            wait(WAIT100ms * (int)pow(2, tries));
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
        wait(WAIT100ms);
    }
    return 0;
}

void KaiSocket::rsync(Message& msg)
{
    while (consume(msg) > 0) {
        if (*(int*)msg.head.text == g_hText) {
            writes(m_network, (uint8_t*)&msg, msg.head.size);
        }
    }
}

int proxyHook(KaiSocket* kai)
{
    if (kai == nullptr) {
        std::cout << "KaiSocket instance is NULL" << std::endl;
        return -1;
    }
    KaiSocket::Message msg = {};
    const size_t Size = sizeof(KaiSocket::Message);
    memset(static_cast<void*>(&msg), 0, Size);
    int len = kai->recv(reinterpret_cast<uint8_t*>(&msg), Size);
    if (len > 0) {
        if (msg.head.etag >= NONE && msg.head.etag <= SUBSCRIBE) {
            std::cout
                << __FUNCTION__ << ": message from " << KaiSocket::G_KaiRole[msg.head.etag]
                << " [" << msg.data.stat << "], MQ detail: '" << msg.head.text
                << "', len = " << len
                << std::endl;
            if (msg.head.etag == CONSUMER) {
                kai->rsync(msg);
            }
        } else {
            std::cout << __FUNCTION__ << ": msg tag = " << msg.head.etag << ": [" << msg.head.size << "]" << std::endl;
        }
    } else {
        std::cout << __FUNCTION__ << ": KaiSocket status = " << len << std::endl;
    }
    return len;
}

int KaiSocket::Broker()
{
    registerCallback(proxyHook);
    return this->start();
}

#if (defined __GNUC__ && __APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"
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
        return -2;
    }
    ssize_t res = ::recv(m_network.socket, reinterpret_cast<char*>(&header), len, 0);
    if (0 == res || (res < 0 && errno != EAGAIN)) { // fixme only when disconnect to continue
        handleNotify(m_network);
        return -3;
    }
#ifdef HEART_BEAT
    if (strncmp(reinterpret_cast<char*>(&header), "Kai", 3) == 0)
        return 0; // heartbeat ignore
#endif
    if (res != len) {
        std::cout << __FUNCTION__ << ": stream caught [" << res << "], expect size = " << len << std::endl;
    }
    std::mutex mtxLck = {};
    std::lock_guard<std::mutex> lock(mtxLck);
    static uint64_t prsid;
    // get ssid set to 'm_network', also repeat to server as a mark for search clients
    unsigned long long ssid = header.ssid;
    for (auto& network : m_networks) {
        if (!m_network.run_01 || m_network.socket == 0)
            continue;
        // select to set consume network
        if (checkSsid(network.socket, ssid)) {
            prsid = ssid;
            network.flag.etag = header.etag;
            memmove(network.flag.text, header.text, sizeof(Header::text));
        }
        wait(1);
    }
    m_network.flag.etag = header.etag;
    m_network.flag.size = header.size;
    memmove(m_network.flag.text, header.text, sizeof(Header::text));
    memmove(buff, &header, len);
    size_t total = m_network.flag.size;
    if (total < sizeof(Message))
        total = sizeof(Message);
    auto* message = new(std::nothrow) uint8_t[total];
    if (message == nullptr) {
        std::cerr << __FUNCTION__ << ": message malloc fail, size = " << total << "!" << std::endl;
        return -4;
    }
    ssize_t left = m_network.flag.size - len;
    ssize_t err = -1;
    if (left > 0) {
        err = ::recv(m_network.socket, reinterpret_cast<char*>(message + len), left, 0);
        if (err <= 0) {
            handleNotify(m_network);
            delete[] message;
            return -5;
        }
    }
    Message msg = *reinterpret_cast<Message*>(buff);
    ssize_t stat = -1;
    switch (msg.head.etag) {
    case PRODUCER:
        stat = produce(msg);
        break;
    case CONSUMER:
        stat = consume(msg);
        if (stat < 0) {
            handleNotify(m_network);
            delete[] message;
            return -6;
        } else {
            break;
        }
    default: break;
    }
    if (msg.head.etag >= NONE && msg.head.etag <= SUBSCRIBE) {
        std::cout << __FUNCTION__ << ": " << G_KaiRole[msg.head.etag] << " operated count = "
            << (stat > 0 ? std::to_string(stat) : "0") << std::endl;
    }
    if (msg.head.etag != 0) {
        using namespace std;
        msg.head.ssid = setSsid(m_network);
        memmove(message, &msg, sizeof(Message));
        bool deal = false;
        for (auto& network : m_networks) {
            if (strcmp(network.flag.text, m_network.flag.text) == 0
#if !defined MULTI_SEND
                && network.flag.ssid == prsid /* comment to multi-send */
#endif
                && network.flag.etag == CONSUMER) { // only consume should be sent
                if ((stat = this->writes(network, message, total)) < 0) {
                    std::cerr << __FUNCTION__ << ": writes to [" << network.socket << "], failure bytes " << total << "!" << std::endl;
                    continue;
                }
                deal = true;
            }
        }
        if (stat >= 0) {
            if (deal)
                strcpy(msg.data.stat, "SUCCESS");
            else {
                memset(msg.head.text, g_hText, 4);
                strcpy(msg.data.stat, "NOTDEAL");
                msg.head.size = total;
                stat = produce(msg);
                if (stat < 0)
                    return stat;
            }
        } else if (stat == -1)
            strcpy(msg.data.stat, "NULLPTR");
        else
            strcpy(msg.data.stat, "FAILURE");
        memmove(buff + HEAD_SIZE, msg.data.stat, sizeof(Message::data.stat));
    } else { // set running is to delete m_network || consume got 0 byte
        handleNotify(m_network);
        std::cerr << __FUNCTION__ << ": unsupported tag [" << msg.head.etag << "]" << std::endl;
        delete[] message;
        return -7;
    }
    delete[] message;
    return (err + res);
}

bool KaiSocket::running()
{
    return m_network.run_01;
}

#if (defined __GNUC__ && __APPLE__)
#pragma GCC diagnostic pop
#endif

void KaiSocket::wait(unsigned int tms)
{
#ifdef USE_SELECT
    const int THOUS = 1000;
    struct timeval delay = {
        .tv_sec = time_t(tms / THOUS),
        .tv_usec = (long)(tms % THOUS * THOUS)
    };
    select(0, NULL, NULL, NULL, &delay);
#else
    std::this_thread::sleep_for(std::chrono::microseconds(tms));
#endif
}

ssize_t KaiSocket::writes(Network& network, const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
        return 0;
    if (errno == EPIPE)
        return -2;
    int left = (int)len;
    auto* buff = new(std::nothrow) uint8_t[left];
    if (buff == nullptr) {
        std::cerr << __FUNCTION__ << ": socket buffer malloc failed!" << std::endl;
        return -1;
    }
    memset(buff, 0, left);
    memcpy(buff, data, left);
    while (left > 0) {
        ssize_t wrote = 0;
        if ((wrote = write(network.socket, reinterpret_cast<char*>(buff + wrote), left)) <= 0) {
            if (wrote < 0) {
                if (errno == EINTR) {
                    wrote = 0; /* call write() again */
                } else {
                    handleNotify(network);
                    delete[] buff;
                    return -3; /* error */
                }
            }
        }
        left -= wrote;
    }
    delete[] buff;
    return ssize_t(len - left);
}

ssize_t KaiSocket::broadcast(const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0) {
        std::cerr << __FUNCTION__ << ": transfer data is null!" << std::endl;
        return -1;
    }
    if (m_networks.empty() || m_networks.begin() == m_networks.end()) {
        std::cerr << __FUNCTION__ << ": no network is to send!" << std::endl;
        return -2;
    }
    std::mutex mtxLck = {};
    std::lock_guard<std::mutex> lock(mtxLck);
    ssize_t bytes = 0;
    for (auto& network : m_networks) {
        ssize_t stat = writes(network, data, len);
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

void KaiSocket::registerCallback(KAI_SOCK_HOOK func)
{
    m_callbacks.clear();
    appendCallback(func);
}

void KaiSocket::appendCallback(KAI_SOCK_HOOK func)
{
    if (std::find(m_callbacks.begin(), m_callbacks.end(), func) == m_callbacks.end()) {
        m_callbacks.emplace_back(func);
    }
}

void KaiSocket::handleNotify(Network& network)
{
    if (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT || errno == EWOULDBLOCK) {
        std::cerr << __FUNCTION__ << ": " << strerror(errno) << std::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    bool exist = false;
    auto at = m_networks.begin();
    for (; at != m_networks.end(); ++at) {
        if (at->socket == network.socket) {
            exist = true;
            break;
        }
    }
    if (!network.run_01) {
        network.run_01 = !network.run_01;
    }
    if (!exist) {
        m_networks.emplace_back(network);
    }
    for (auto it = m_networks.begin(); it != m_networks.end();) {
        if (it->socket < 0 || m_network.socket < 0 || m_networks.empty())
            break;
        std::stringstream hint;
        hint
            << "### " << (m_network.client ? "Server" : "Client")
            << "(" << it->IP << ":" << it->PORT << ") socket ["
            << it->socket << "] lost.";
        if ((it->socket == network.socket && network.run_01) || errno == EBADF) {
            it->run_01 = false;
            close(it->socket);
            static SOCKET socket = 0;
            if (socket != it->socket) {
                std::cerr << hint.str() << std::endl;
            }
            socket = it->socket;
            if (m_networks.size() == 1) {
                m_networks.clear();
                break;
            } else {
                it = m_networks.erase(it);
                continue;
            }
        }
        ++it;
        wait(1);
    }
}

void KaiSocket::runCallback(KaiSocket* sock, KAI_SOCK_HOOK func)
{
    if (sock == nullptr)
        return;
    if (sock->m_network.flag.etag != PRODUCER) {
        sock->m_network.run_01 = true; // server
    }
    for (auto& network : sock->m_networks) {
        if (network.client && !g_thrStat) {
            // heartBeat
            std::thread([=](Network& work, KaiSocket* kai) {
                while (kai->running()) {
                    if (::send(work.socket, "Kai", 3, 0) <= 0) {
                        std::cerr << "Heartbeat to " << work.IP << ":"
                            << work.PORT << " arrests." << std::endl;
                        kai->handleNotify(work);
                        break;
                    }
                    KaiSocket::wait(30000); // frequency 30s
                } }, std::ref(network), sock).detach();
                g_thrStat = !g_thrStat;
        }
    }
    while (sock->running()) {
        wait(WAIT100ms);
        if (func == nullptr) {
            continue;
        }
        int stat = func(sock);
        if (stat <= 0) {
            std::cout << "callback stat = [" << stat << "," << errno << "," << strerror(errno) << "]" << std::endl;
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
    return ((uint64_t)network.PORT << 16 | (uint64_t)socket << 8 | ip);
}

bool KaiSocket::checkSsid(SOCKET key, uint64_t ssid)
{
    std::lock_guard<std::mutex> lock(m_lock);
    return ((int)((ssid >> 8) & 0x00ff) == key);
}

#ifdef FULLY_COMPILE
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
        if (sock == nullptr) {
            std::cerr << "KaiSocket instance is null" << std::endl;
            return -1;
        }
        func(data, size);
        size_t len = sock->send(data, size);
        if (len != size) {
            std::cerr << "Sent size/len (" << len << ", " << size << ") mismatch." << std::endl;
        }
        return size = len;
    };
    appendCallback(hook);
}
#endif

int KaiSocket::produce(const Message& msg)
{
    if (m_msgQue == nullptr) {
        std::cout << __FUNCTION__ << ": msgQue pool is null." << std::endl;
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    size_t size = m_msgQue->size();
    if (msg.head.ssid != 0 || msg.head.etag == PRODUCER) {
        // Message* mess = new(msg)Message();
        m_msgQue->emplace_back(&msg);
    }
    return static_cast<int>(m_msgQue->size() - size);
}

ssize_t KaiSocket::consume(Message& msg)
{
    if (m_msgQue == nullptr || m_msgQue->empty()) {
        std::cout << __FUNCTION__ << ": message pool has no elem(s)." << std::endl;
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_lock);
    size_t size = m_msgQue->size();
    Message* msgQ = nullptr;
    do {
        try {
            msgQ = const_cast<Message*>(m_msgQue->front());
            if (msgQ->head.etag != CONSUMER) {
                continue;
            }
            msg.head = msgQ->head; // fixme memmove_avx_unaligned_erms
            memmove(&msg.data, &msgQ->data, sizeof(Message::Payload));
        } catch (const std::exception& e) {
            std::cerr << __FUNCTION__ << ": segmentation fault: " << e.what() << std::endl;
        }
        if (size > 0) {
            m_msgQue->pop_front();
        }
    } while (msgQ == nullptr);
    // if 1: success, 0: nothing
    return static_cast<int>(size - m_msgQue->size());
}

ssize_t KaiSocket::send(const uint8_t* data, size_t len)
{
    return broadcast(data, len);
}

void KaiSocket::finish()
{
    for (auto& network : m_networks) {
        network.run_01 = false;
        while (network.run_01) {
            wait(WAIT100ms);
        }
        close(network.socket);
    }
    {
        m_networks.clear();
        m_callbacks.clear();
    }
    if (m_msgQue != nullptr) {
        for (auto& msg : *m_msgQue) {
            if (msg != nullptr)
                delete msg;
        }
        delete m_msgQue;
        m_msgQue = nullptr;
    }
}

std::string KaiSocket::getFile2string(const std::string& filename)
{
    std::string s{};
    FILE* fp = fopen(filename.c_str(), "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        s.resize(len);
        fread((void*)(s.c_str()), 1, len, fp);
        fclose(fp);
    } else {
        std::cerr << __FUNCTION__ << ": file [" << filename << "] open failed: " << strerror(errno) << std::endl;
    }
    return s;
}

void KaiSocket::setTopic(const std::string& topic, Header& header)
{
    std::lock_guard<std::mutex> lock(m_lock);
    size_t size = topic.size();
    if (size > sizeof(header.text)) {
        std::cerr << __FUNCTION__ << ": topic length " << size << " out of bounds." << std::endl;
        size = sizeof(header.text);
    }
    memmove(header.text, topic.c_str(), size);
    memmove(m_network.flag.text, header.text, size);
    m_network.flag.etag = header.etag;
}

ssize_t KaiSocket::Subscriber(const std::string& message, CALLBACK_RCV callback)
{
    if (this->connect() < 0)
        return -2;
    Message msg = {};
    const size_t Size = HEAD_SIZE + sizeof(Message::Payload::stat);
    volatile bool flag = false;
    do {
        memset(static_cast<void*>(&msg), 0, Size);
        ssize_t len = ::recv(m_network.socket, reinterpret_cast<char*>(&msg), Size, 0);
        if (len < 0 || (len == 0 && errno == EINVAL)) {
            std::cerr << __FUNCTION__ << ": recv head fail, " << strerror(errno) << std::endl;
            handleNotify(m_network);
            return -3;
        }
        char* kai = (char*)(&msg);
        if (kai[0] == 'K' && kai[1] == 'a' && kai[2] == 'i')
            continue;
        static long count = 0;
        flag = (msg.head.ssid != 0 || len == 0);
        if (msg.head.size == 0) {
            msg.head.size = Size;
            msg.head.etag = CONSUMER;
            if (msg.head.ssid == 0) {
                msg.head.ssid = setSsid(m_network);
            }
            // parse message divide to topic/etc...
            const std::string& topic = message; // "message.sub()...";
            setTopic(topic, msg.head);
            len = writes(m_network, (uint8_t*)&msg, Size);
            if (len < 0) {
                std::cerr << __FUNCTION__ << ": writes " << strerror(errno) << std::endl;
                return -4;
            }
            std::cout << __FUNCTION__ << " process run as " << KaiSocket::G_KaiRole[msg.head.etag]
                << ", MQ(" << count << ") topic is '" << msg.head.text << "'." << std::endl;
            count++;
        } else {
            count = 0;
        }
        if (msg.head.size > Size) {
            size_t remain = msg.head.size - Size;
            auto* body = new(std::nothrow) char[remain];
            if (body == nullptr) {
                std::cerr << __FUNCTION__ << ": body malloc failed!" << std::endl;
                return -1;
            }
            len = ::recv(m_network.socket, body, remain, 0);
            if (len < 0) {
                std::cerr << __FUNCTION__ << ": recv body fail, " << strerror(errno) << std::endl;
                handleNotify(m_network);
                delete[] body;
                return -5;
            } else {
                msg.data.stat[0] = 'O';
                msg.data.stat[1] = 'K';
                msg.data.stat[2] = '\0';
                Message* pMsg = reinterpret_cast<Message*>(new char[sizeof(Message) + len]);
                if (pMsg != nullptr) {
                    memcpy(pMsg, &msg, sizeof(Message));
                    if (len > 0) {
                        memcpy(pMsg->data.body, body, len);
                        pMsg->data.body[len] = '\0';
                    }
                    if (callback != nullptr) {
                        callback(*pMsg);
                    }
                    std::cout << __FUNCTION__ << ": message payload(" << (len + Size) << ") = [" << pMsg->data.stat << "]-[" << pMsg->data.body << "]" << std::endl;
                    delete[] pMsg;
                }
            }
            delete[] body;
        }
    } while (flag);
#ifdef TEST_CONSUME
    if (m_networks.size() == 0 || m_networks.begin() == m_networks.end())
        return -1;
    for (std::vector<Network>::iterator it = m_networks.begin(); it != m_networks.end(); ++it) {
        if (strcmp(it->flag.topic, m_network.flag.topic) == 0 && it->socket != m_network.socket) {
            Message msg = {};
            if (consume(msg) >= 0) {
                if (::send(it->socket, message.c_str(), message.size(), 0) <= 0) {
                    handleNotify(*it);
                    it = m_networks.begin();
                    continue;
                }
            } else {
                std::cerr << "Consume fail, socket = " << it->socket << ", topic: "
                    << msg.head.topic << "(" << it->flag.topic << ")" << std::endl;
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
        std::cerr << __FUNCTION__ << ": topic/payload was empty!" << std::endl;
        return -3;
    } else {
        this->m_network.run_01 = false;
        if (!m_callbacks.empty()) m_callbacks.clear();
        g_maxTimes = 0;
    }
    std::size_t maxLen = payload.max_size();
    Message msg = {};
    memset(static_cast<void*>(&msg), 0, sizeof(Message));
    size = (size > maxLen ? maxLen : size);
    size_t msgLen = sizeof msg + size;
    msg.head.size = static_cast<unsigned int>(msgLen);
    msg.head.etag = PRODUCER;
    setTopic(topic, msg.head);
    if (this->connect() != 0) {
        std::cerr << __FUNCTION__ << ": unable to connect!" << std::endl;
        return -2;
    }
    auto* message = new(std::nothrow) uint8_t[msgLen];
    if (message == nullptr) {
        std::cerr << __FUNCTION__ << ": message malloc failed!" << std::endl;
        return -1;
    }
    memmove(message, &msg, sizeof(Message));
    memmove(message + HEAD_SIZE + sizeof(Message::Payload::stat), payload.c_str(), size);
    ssize_t len = this->broadcast(message, msgLen);
    delete[] message;
    finish();
    return len;
}
