#include "KaSocket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>

KaSocket::KaSocket(unsigned short srvport)
{
    new (this)KaSocket(nullptr, srvport); // placement new
}

KaSocket::KaSocket(const char * srvip, unsigned short servport)
{
    if (srvip != nullptr)
        current.IP = srvip;
    current.socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (current.socket < 0) {
        std::cerr
            << "Creating socket (" << (errno != 0 ? strerror(errno) : std::to_string(current.socket)) << ")."
            << std::endl;
        return;
    }
    current.PORT = servport;
}

int KaSocket::start()
{
    int listen_socket = current.socket;
    unsigned short servport = current.PORT;

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
        int rcv_sock = current.socket = ::accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
        if (rcv_sock < 0) {
            std::cerr
                << "Socket accept (" << (errno != 0 ? strerror(errno) : std::to_string(rcv_sock)) << ")."
                << std::endl;
            return -3;
        };

        time_t t;
        bool set = true;
        struct tm * lt;
        {
            std::mutex mtxlck;
            std::lock_guard<std::mutex> lock(mtxlck);
            time(&t);
            lt = localtime(&t);
            g_threadNo_++;
            current.run_ = true;
            setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
            getpeername(rcv_sock, reinterpret_cast<struct sockaddr *>(&peerAddr), &peerLen);
            current.IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
            current.PORT = ntohs(peerAddr.sin_port);
            fprintf(stdout, "accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n",
                g_threadNo_,
                current.IP.c_str(), current.PORT,
                lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

            Head head{ 0, 0, current.flag.ssid = setSsid(current, rcv_sock), 0 };
            ::send(rcv_sock, (char*)&head, sizeof(Head), 0);
            networks.emplace_back(current);

            std::cout << "socket monitor: " << rcv_sock << "; waiting for massage." << std::endl;
            for (std::vector<void(*)(void*)>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
                std::thread(&KaSocket::runCallback, this, this, (*it)).detach();
            }
            wait(100);
        }
    }
    return 0;
}

int KaSocket::connect()
{
    sockaddr_in srvaddr;
    const char* ipaddr = current.IP.c_str();
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
    srvaddr.sin_port = htons(current.PORT);
    std::cout << "------ client v0.1: " << ipaddr << " ------" << std::endl;
    bool bReuseaddr;
    setsockopt(current.socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
    int tries = 0;
    while (::connect(current.socket, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == (-1)) {
        if (tries < 100) {
            wait(100);
            tries++;
        } else {
            std::cerr << "Trying to connect (" << "tries=" << tries << ", "
                << (errno != 0 ? strerror(errno) : "No error") << ")." << std::endl;
            handleNotify(current.socket);
            return -1;
        }
    }
    current.run_ = true;
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
    memcpy(mesg, &current.flag, head);
    memcpy(mesg + head, data, len);
    const char *ptr = mesg;
    while (left > 0) {
        ssize_t wrote;
        if ((wrote = write(current.socket, ptr, left)) <= 0) {
            if (wrote < 0 && errno == EINTR)
                wrote = 0; /* call write() again */
            else {
                handleNotify(current.socket);
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
    if (networks.size() == 0 || networks.begin() == networks.end())
        return -1;
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        int head = sizeof(Head);
        int size = head + len;
        char mesg[size];
        memset(mesg, 0, size);
        memcpy(mesg, &it->flag, head);
        memcpy(mesg + head, data, len);
        int error = ::send(it->socket, mesg, size, 0);
        if (error <= 0 &&
            errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
            handleNotify(it->socket);
            continue;
        }
        wait(1);
    }
    return 0;
}

int KaSocket::transfer(const char * data, int len)
{ 
    if (networks.size() == 0 || networks.begin() == networks.end())
        return -1;
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        if (memcmp(it->flag.mqid, current.flag.mqid, sizeof(Head::mqid) == 0) 
            && it->socket != current.socket) {
            if (::send(it->socket, data, len, 0) <= 0 &&
                errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
                handleNotify(it->socket);
                continue;
            }
            wait(1);
        }
    }
    return 0;
}

int KaSocket::recv(char * buff, int len)
{
    int size = sizeof(Head);
    char header[size];
    memset(header, 0, size);
    if (server && networks.size() == 0) {
        handleNotify(current.socket);
        return -1;
    }
    int res = ::recv(current.socket, header, size, 0);
    // get ssid set to 'current', also repeat to server as a mark for search clients
    if (res == size) {
        std::mutex mtxlck;
        std::lock_guard<std::mutex> lock(mtxlck);
        unsigned long long ssid = reinterpret_cast<Head*>(header)->ssid;
        if (ssid != 0) {
            current.flag.ssid = ntohl(ssid);
            strcpy(current.flag.mqid, reinterpret_cast<Head*>(header)->mqid);
            for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
                if (&(*it) == nullptr || &current == nullptr)
                    continue;
                if (verifySsid((*it), current.flag.ssid)) {
                    it->flag.ssid = setSsid(current, it->socket);
                }
                wait(1);
            }
            memcpy(buff, header, size);
        } else {
            size = 0;
        }
    } else {
        if (header[0] == 'K' && header[1] == 'a')
            return 0;
    }
    int err = ::recv(current.socket, buff + size, len, 0);
    if (err <= 0 &&
        errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
        handleNotify(current.socket);
        err = -2;
    }
    return err + res;
}

bool KaSocket::running()
{
    if (!this)
        return false;
    return current.run_;
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

void KaSocket::handleNotify(int socket)
{
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        //FIXME: signal SIGSEGV, Segmentation fault.
        std::mutex mtxlck;
        std::lock_guard<std::mutex> lock(mtxlck);
        if (socket < 0 || it->socket < 0 || networks.size() == 0)
            return;
        if (it->socket == socket && it->run_) {
            {
                std::vector<Network>::iterator iter = it;
                it->run_ = false;
                std::cerr
                    << "### " << (server ? "Client" : "Server")
                    << "(" << it->IP << ":" << it->PORT << ") socket [" << it->socket << "] lost."
                    << std::endl;
                if (networks.size() > 0) {
                    close(it->socket);
                    it = networks.erase(iter);
                    if (it != networks.begin())
                        --it;
                    if (it == networks.end())
                        break;
                } else break;
            }
        }
        wait(1);
    }
}

KaSocket::~KaSocket()
{
    close(current.socket);
}

void KaSocket::runCallback(KaSocket* sock, void(*func)(void *))
{
    if (func != nullptr) {
        func(sock);
        // heartBeat
        std::thread(
            [](Network& current)
        {
            while (1) {
                if (::send(current.socket, "Ka", 2, 0) <= 0 &&
                    errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR && errno != ETIMEDOUT) {
                    std::cerr << "Heartbeat to " << current.IP << ":" << current.PORT << " arrests." << std::endl;
                    break;
                }
                KaSocket::wait(3000);
            }
        }, std::ref(current)).detach();
    }
}

unsigned long long KaSocket::setSsid(Network current, int socket)
{
    unsigned int ip = 0;
    const char* s = reinterpret_cast<char*>((unsigned char**)&current.IP);
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
    return (current.PORT << 16 | socket << 8 | ip);
}

bool KaSocket::verifySsid(Network current, unsigned long long ssid)
{
    return ((int)((ssid >> 8) & 0x00ff) == current.socket);
}

void KaSocket::setMqid(std::string mqid)
{
    memcpy(current.flag.mqid, mqid.c_str(), sizeof(Head::mqid));
}

int KaSocket::produce(Message& msg)
{
    if (msgque == nullptr)
        return -1;
    if (&msg != nullptr)
        msgque->push_back(&msg);
    return msgque->size();
}

int KaSocket::consume()
{
    if (msgque == nullptr)
        return -1;
    if (msgque->empty())
        return 0;
    msgque->pop_front();
    return msgque->size();
}
