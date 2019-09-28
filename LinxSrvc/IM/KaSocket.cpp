#include "KaSocket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>

static unsigned int g_threadNo_ = 0;

KaSocket::KaSocket(unsigned short srvport)
{
    new (this)KaSocket(nullptr, srvport); // placement new
}

KaSocket::KaSocket(const char * srvip, unsigned short servport)
{
    if (srvip != nullptr)
        network.IP = srvip;
    network.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (network.socket < 0) {
        std::cerr
            << "Creating socket (" << (errno != 0 ? strerror(errno) : std::to_string(network.socket)) << ")."
            << std::endl;
        return;
    }
    network.PORT = servport;
}

int KaSocket::start()
{
    int listen_socket = network.socket;
    unsigned short servport = network.PORT;

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

    if (listen(listen_socket, 50) < 0) {
        std::cerr
            << "Socket listen (" << (errno != 0 ? strerror(errno) : std::to_string(listen_socket)) << ")."
            << std::endl;
        close(listen_socket);
        return -2;
    }

    struct sockaddr_in listenAddr;
    socklen_t listenLen = static_cast<socklen_t>(sizeof(listenAddr));
    getsockname(listen_socket, reinterpret_cast<struct sockaddr *>(&listenAddr), &listenLen);
    std::cout << "localhost listening [" << inet_ntoa(listenAddr.sin_addr) << ":" << servport << "]." << std::endl;

    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peerAddr;
    socklen_t peerLen = static_cast<socklen_t>(sizeof(peerAddr));
    server = true;

    while (1) {
        struct sockaddr_in sin;
        socklen_t len = static_cast<socklen_t>(sizeof(sin));
        int rcv_sock = network.socket = ::accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
        if (rcv_sock < 0) {
            std::cerr
                << "Socket accept (" << (errno != 0 ? strerror(errno) : std::to_string(rcv_sock)) << ")."
                << std::endl;
            return -3;
        };

        time_t t;
        struct tm * lt;
        time(&t);
        lt = localtime(&t);
        g_threadNo_++;
        bool set = true;
        setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
        getpeername(rcv_sock, reinterpret_cast<struct sockaddr *>(&peerAddr), &peerLen);
        network.IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
        network.PORT = ntohs(peerAddr.sin_port);
        fprintf(stdout, "accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n",
            g_threadNo_,
            network.IP.c_str(), network.PORT,
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

        network.run_ = true;
        networks.emplace_back(network);
        std::cout << "socket monite: " << rcv_sock << "; waiting for massage." << std::endl;

        Head head{ 0, /*network.head.ssid = */setSsid(network, rcv_sock), 0 };
        ::send(rcv_sock, (char*)&head, sizeof(Head), 0);

        // std::thread(&KaSocket::heartBeat, this, network).detach();

        for (std::vector<void(*)(void*)>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
            std::thread(&KaSocket::runCallback, this, this, (*it)).detach();
        }
        wait(100);
    }
    return 0;
}

int KaSocket::connect()
{
    sockaddr_in srvaddr;
    const char* ipaddr = network.IP.c_str();
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
    srvaddr.sin_port = htons(network.PORT);
    std::cout << "------ client v0.1: " << ipaddr << " ------" << std::endl;
    bool bReuseaddr;
    setsockopt(network.socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
    int tries = 0;
    while (::connect(network.socket, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == (-1)) {
        if (tries < 100) {
            wait(100);
            tries++;
        } else {
            std::cerr << "Trying to connect (" << "tries=" << tries << ", "
                << (errno != 0 ? strerror(errno) : "No error") << ")." << std::endl;
            notify(network.socket);
            return -1;
        }
    }
    network.run_ = true;
    for (std::vector<void(*)(void*)>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
        std::thread(&KaSocket::runCallback, this, this, (*it)).detach();
    }
    return 0;
}

int KaSocket::send(const char * data, int len)
{
    int head = sizeof(Head);
    size_t left = len + head;
    char mesg[left];
    memset(mesg, 0, left);
    memcpy(mesg, &network.head, head);
    memcpy(mesg + head, data, len);
    const char *ptr = mesg;
    while (left > 0) {
        ssize_t wrote;
        if ((wrote = write(network.socket, ptr, left)) <= 0) {
            if (wrote < 0 && errno == EINTR)
                wrote = 0; /* call write() again */
            else {
                notify(network.socket);
                return -1; /* error */
            }
        }
        left -= wrote;
        ptr += wrote;
    }
    return 0;
}

int KaSocket::broadcast(const char * data, int len)
{
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        int head = sizeof(Head);
        int size = head + len;
        char mesg[size];
        memset(mesg, 0, size);
        memcpy(mesg, &it->head, head);
        memcpy(mesg + head, data, len);
        int error = ::send(it->socket, mesg, size, 0);
        if (error <= 0 &&
            errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
            notify(it->socket);
            return -1;
        }
        wait(1);
    }
    return 0;
}

int KaSocket::recv(char * buff, int len)
{
    int error = 0;
    int size = sizeof(Head);
    char header[size];
    memset(header, 0, size);
    if (server && networks.size() == 0) {
        notify(network.socket);
        return -1;
    }
    int flag = ::recv(network.socket, header, size, 0);
    if (header[0] == 'K' && header[1] == 'a') {
        return 0;
    }
    // get ssid set to 'network', also repeat to server as a mark for search clients
    if (flag == size) {
        std::lock_guard<std::mutex> lock(mtxlck);
        unsigned long long ssid = reinterpret_cast<Head*>(header)->ssid;
        if (ssid != 0) {
            network.head.ssid = ssid;
            for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
                if (&(*it) == nullptr || &network == nullptr)
                    continue;
                if (verifySsid((*it), network.head.ssid)) {
                    it->head.ssid = setSsid(network, it->socket);
                }
                wait(1);
            }
            memcpy(buff, header, size);
        } else {
            size = 0;
        }
    }
    error = ::recv(network.socket, buff + size, len, 0);
    if (error <= 0 &&
        errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
        notify(network.socket);
        error = -2;
    }
    return error;
}

bool KaSocket::running()
{
    if (!this)
        return false;
    return network.run_;
}

void KaSocket::wait(unsigned int tms)
{
    struct timeval time;
    time.tv_sec = tms / 1000;
    time.tv_usec = tms % 1000 * 1000;
    select(0, NULL, NULL, NULL, &time);
}

void KaSocket::setCallback(void(*func)(void *))
{
    callbacks.clear();
    addCallback(func);
}

void KaSocket::addCallback(void(*func)(void *))
{
    if (std::find(callbacks.begin(), callbacks.end(), func) == callbacks.end()) {
        callbacks.emplace_back(func);
    }
}

void KaSocket::notify(int socket)
{
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        if (socket < 0 || it->socket < 0 || networks.size() == 0)
            return;
        if (it->socket == socket && it->run_) {
            {
                //FIXME: signal SIGSEGV, Segmentation fault.
                std::lock_guard<std::mutex> lock(mtxlck);
                std::vector<Network>::iterator iter = it;
                it->run_ = false;
                std::cerr
                    << "### " << (server ? "Client" : "Server")
                    << "(" << it->IP << ":" << it->PORT << ") socket [" << it->socket << "] lost."
                    << std::endl;
                if (networks.size() > 0) {
                    close(it->socket);
                    it = networks.erase(iter);
                    it--;
                } else break;
            }
        }
        wait(1);
    }
}

KaSocket::~KaSocket()
{
    close(network.socket);
}

void KaSocket::runCallback(KaSocket* sock, void(*func)(void *))
{
    if (func != nullptr) {
        func(sock);
    }
}

void KaSocket::heartBeat(Network& network)
{
    while (1) {
        if (::send(network.socket, "Ka", 2, 0) <= 0 &&
            errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
            std::cerr << "Heartbeat to " << network.IP << ":" << network.PORT << " arrests." << std::endl;
            break;
        }
        wait(3000);
    }
}

unsigned long long KaSocket::setSsid(Network network, int socket)
{
    unsigned int ip = 0;
    const char* s = reinterpret_cast<char*>((unsigned char**)&network.IP);
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
    return (network.PORT << 16 | socket << 8 | ip);
}

bool KaSocket::verifySsid(Network network, unsigned long long ssid)
{
    return (((ssid >> 8) & 0x00ff) == network.socket);
}
