#include "KaiSocket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <thread>

KaiSocket::KaiSocket(unsigned short srvport)
{
    new (this)KaiSocket(nullptr, srvport); // placement new
}

KaiSocket::KaiSocket(const char* srvip, unsigned short servport)
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

int KaiSocket::start()
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
    getsockname(listen_socket, reinterpret_cast<struct sockaddr*>(&listenAddr), &listenLen);
    std::cout << "localhost listening [" << inet_ntoa(listenAddr.sin_addr) << ":" << servport << "]." << std::endl;

    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peerAddr;
    socklen_t peerLen = static_cast<socklen_t>(sizeof(peerAddr));
    m_clientMode = false;

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
        {
            std::mutex mtxlck;
            std::lock_guard<std::mutex> lock(mtxlck);
            bool set = true;
            time_t t;
            time(&t);
            struct tm* lt = localtime(&t);
            g_threadNo_++;
            current.run_ = true;
            setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
            getpeername(rcv_sock, reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);
            current.IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
            current.PORT = ntohs(peerAddr.sin_port);
            fprintf(stdout, "accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n",
                g_threadNo_,
                current.IP.c_str(), current.PORT,
                lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

            Header head{ 0, 0, current.flag.ssid = setSsid(current, rcv_sock), 0 };
            ::send(rcv_sock, (char*)&head, sizeof(Header), 0);
            networks.emplace_back(current);

            std::cout << "socket monitor: " << rcv_sock << "; waiting for massage." << std::endl;
            for (std::vector<int(*)(KaiSocket*)>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
                std::thread(&KaiSocket::runCallback, this, this, (*it)).detach();
            }
            wait(100);
        }
    }
    return 0;
}

int KaiSocket::connect()
{
    const char* ipaddr = current.IP.c_str();
    sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
    srvaddr.sin_port = htons(current.PORT);
    std::cout << "------ client v0.1: " << ipaddr << " ------" << std::endl;
    bool bReuseaddr = false;
    setsockopt(current.socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bool));
    int tries = 0;
    while (::connect(current.socket, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == (-1)) {
        if (tries < WAIT_TIME) {
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
    for (std::vector<int(*)(KaiSocket*)>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
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
    memcpy(mesg, &current.flag, head);
    memcpy(mesg + head, data, len);
    const char* ptr = mesg;
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
    return (len + head - left);
}

int KaiSocket::broadcast(const char* data, int len)
{
    if (networks.size() == 0 || networks.begin() == networks.end())
        return -1;
    int bytes = 0;
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
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
    int res = ::recv(current.socket, data, len, 0);
    if (res < 0) {
        handleNotify(current.socket);
        return -1;
    }
    if (data[0] == 'K' && data[1] == 'a' && data[2] == 'i')
        return 0;
    if (res == 0 || networks.size() == 1)
        return res;
    if (networks.size() == 0 || networks.begin() == networks.end())
        return -2;
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        if (strcmp(it->flag.mqid, current.flag.mqid) == 0 && it->socket != current.socket) {
            if (::send(it->socket, data, res, 0) <= 0) {
                handleNotify(it->socket);
                it = networks.begin();
                continue;
            }
            wait(1);
        }
    }
    return res;
}

int KaiSocket::recv(char* buff, int len)
{
    if (buff == nullptr || len == 0)
        return 0;
    int size = sizeof(Header);
    char header[size];
    memset(header, 0, size);
    if (!m_clientMode && networks.size() == 0) {
        handleNotify(current.socket);
        return -1;
    }
    int res = ::recv(current.socket, header, size, 0);
    if (res < 0) {
        handleNotify(current.socket);
        return -2;
    }
    // get ssid set to 'current', also repeat to server as a mark for search clients
    if (res == size) {
        std::mutex mtxlck = {};
        std::lock_guard<std::mutex> lock(mtxlck);
        unsigned long long ssid = reinterpret_cast<Header*>(header)->ssid;
        if (ssid != 0) {
            current.flag.ssid = ntohl(ssid);
            memcpy(current.flag.mqid, reinterpret_cast<Header*>(header)->mqid, 32);
            for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
                if (&(*it) == nullptr || &current == nullptr)
                    continue;
                if (verifySsid((*it), current.flag.ssid)) {
                    it->flag.ssid = setSsid(current, it->socket);
                    strcpy(it->flag.mqid, current.flag.mqid);
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
    int err = ::recv(current.socket, buff + size, len, 0);
    if (err <= 0) {
        handleNotify(current.socket);
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
    return err + res;
}

bool KaiSocket::running()
{
    if (!this)
        return false;
    return current.run_;
}

void KaiSocket::wait(unsigned int tms)
{
    const int THOS = 1000;
    struct timeval delay = {
        .tv_sec = time_t(tms / THOS),
        .tv_usec = (long)(tms % THOS * THOS)
    };
    select(0, NULL, NULL, NULL, &delay);
}

void KaiSocket::registCallback(int(*func)(KaiSocket*))
{
    callbacks.clear();
    appendCallback(func);
}

void KaiSocket::appendCallback(int(*func)(KaiSocket*))
{
    if (std::find(callbacks.begin(), callbacks.end(), func) == callbacks.end()) {
        callbacks.emplace_back(func);
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
        if (len == size)
            func(data, size);
        else if (len != 0)
            std::cerr << "Recieved size/len (" << len << ", " << size << ") not match." << std::endl;
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
        if (len != size)
            std::cerr << "Sent size/len (" << len << ", " << size << ") not match." << std::endl;
        size = len;
        return size;
    };
    appendCallback(hook);
}

void KaiSocket::handleNotify(int socket)
{
    if (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT || errno == EWOULDBLOCK)
        return;
    for (std::vector<Network>::iterator it = networks.begin(); it != networks.end(); ++it) {
        //FIXME: signal SIGSEGV, Segmentation fault.
        std::mutex mtxlck = {};
        std::lock_guard<std::mutex> lock(mtxlck);
        if (socket < 0 || it->socket < 0 || networks.size() == 0)
            return;
        if (it->socket == socket && it->run_) {
            {
                std::vector<Network>::iterator iter = it;
                it->run_ = false;
                std::cerr
                    << "### " << (m_clientMode ? "Server" : "Client")
                    << "(" << it->IP << ":" << it->PORT << ") socket [" << it->socket << "] lost."
                    << std::endl;
                if (networks.size() > 0) {
                    close(it->socket);
                    if (networks.size() == 1) {
                        networks.clear();
                        break;
                    }
                    if (networks.end() == networks.erase(iter)) return;
                    it = networks.begin();
                } else return;
            }
        }
        wait(1);
    }
}

KaiSocket::~KaiSocket()
{
    close(current.socket);
}

void KaiSocket::runCallback(KaiSocket* sock, int (*func)(KaiSocket*))
{
    if (m_clientMode && !thdref) {
        // heartBeat
        std::thread(
            [](Network& current, KaiSocket* sock)
            {
                while (sock->running()) {
                    if (::send(current.socket, "Kai", 3, 0) <= 0) {
                        std::cerr << "Heartbeat to " << current.IP << ":" << current.PORT << " arrests." << std::endl;
                        sock->handleNotify(current.socket);
                        break;
                    }
                    KaiSocket::wait(30000);
                }
            }, std::ref(current), sock).detach();
            thdref = !thdref;
    }
    if (func != nullptr) {
        while (sock->running()) {
            int ret = func(sock);
            if (ret != 0)
                std::cerr << "Callback status = " << ret << std::endl;
        }
    }
}

unsigned long long KaiSocket::setSsid(Network current, int socket)
{
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
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

bool KaiSocket::verifySsid(Network current, unsigned long long ssid)
{
    return ((int)((ssid >> 8) & 0x00ff) == current.socket);
}

int KaiSocket::produceClient(std::string body, ...)
{
    Message msg = {};
    memset(&msg, 0, sizeof(Message));
    current.flag.etag = msg.head.etag = 1;
    int size = body.size();
    memcpy(msg.data.body, body.c_str(), size > 256 ? 256 : size);
    int res = this->send((char*)&msg, sizeof(Message));
    if (res < 0)
        return -1;
    return res;
}

int KaiSocket::consumeClient() {
    Message msg = {};
    memset(&msg, 0, sizeof(Message));
    current.flag.etag = msg.head.etag = 2;
    return this->send((char*)&msg, sizeof(Message));
}

int KaiSocket::produce(Message& msg)
{
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
    if (msgque == nullptr)
        return -1;
    int size = msgque->size();
    if (&msg != nullptr)
        msgque->emplace_back(new Message(msg));
    return msgque->size() - size - 1;
}

int KaiSocket::consume(Message& msg)
{
    std::mutex mtxlck = {};
    std::lock_guard<std::mutex> lock(mtxlck);
    if (msgque == nullptr)
        return -1;
    if (msgque->empty())
        return -2;
    int size = msgque->size();
    Message* mesg = msgque->front();
    memcpy(&msg.head, &mesg->head, sizeof(Header));
    memcpy(&msg.data, &mesg->data, sizeof(Message::Payload));
    msgque->pop_front();
    if (mesg != nullptr)
        delete mesg;
    return size - msgque->size() - 1;
}
