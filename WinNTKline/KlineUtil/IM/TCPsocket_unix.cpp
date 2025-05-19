#include <stdio.h>         // printf, fprintf, perror
#include <stdint.h>        // uint8_t, uint32_t
#include <stdlib.h>        // atoi
#include <string.h>        // memset, strerror
#include <errno.h>         // errno
#include <unistd.h>        // close
#include <fcntl.h>         // O_CREAT, O_RDWR, O_APPEND, open, write
#include <sys/types.h>     // ssize_t, socklen_t
#include <sys/socket.h>    // socket, bind, listen, accept, recv, send, sockaddr, SOL_SOCKET, SO_REUSEADDR
#include <netinet/in.h>    // sockaddr_in, INADDR_ANY, htons, ntohs
#include <arpa/inet.h>     // inet_ntop
#include <signal.h>        // signal, SIGINT
#include <thread>          // std::thread
#include <chrono>          // std::chrono
#include <atomic>          // std::atomic
#include <algorithm>       // std::max

typedef int (*CALLBACK)(uint8_t*, uint32_t);
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define PRINT_RECV(txt, len) do { \
    fprintf(stdout, "----[ "); \
    for (int c = 0; c < (len); c++) { \
        if (c > 0 && c % 32 == 0) fprintf(stdout, "\n"); \
        fprintf(stdout, "%02x ", static_cast<unsigned char>((txt)[c])); \
    } \
    fprintf(stdout, "]----\n"); \
} while (0)

struct Sockets {
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in local { };
};

constexpr ssize_t BuffSize = 1024;
CALLBACK g_callback = nullptr;
static int g_fld = -1;
std::atomic<bool> g_status{ false };
std::atomic<size_t> g_rcvdBytes{ 0 };
static auto g_lastTime = std::chrono::high_resolution_clock::now();

void sigHandle(int)
{
    g_status = false;
    printf("exit!\n");
}

Sockets setup(short port)
{
    Sockets socks{};
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("invalid socket");
        return socks;
    }
    printf("begin socket setup\n");
    struct sockaddr_in local { };
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    printf("start bind %d\n", port);
    if (bind(sock, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
        perror("bind failed");
        close(sock);
        return socks;
    }
    printf("listening...\n");
    if (listen(sock, 5) != 0) {
        perror("listen failed");
        close(sock);
        return socks;
    }
#ifdef SAVE_DATA
    g_fld = open("dump.bin", O_CREAT | O_RDWR | O_APPEND, 0644);
#endif
    socks.sock = sock;
    socks.local = local;
    g_status = true;

    signal(SIGINT, sigHandle);

    std::thread([] {
        using namespace std::chrono;
        auto lastTime = g_lastTime;
        size_t lastBytes = 0;
        while (g_status) {
            auto current = high_resolution_clock::now();
            duration<double> interval = current - lastTime;
            if (interval.count() >= 1.0) {
                size_t bytesReceived = g_rcvdBytes - lastBytes;
                printf("Speed: %.2f bytes/sec\n", bytesReceived / interval.count());
                lastBytes = g_rcvdBytes;
                lastTime = current;
            }
            std::this_thread::sleep_for(milliseconds(100));
        }
        }).detach();

    return socks;
}

void RegisterCallback(CALLBACK callback)
{
    g_callback = callback;
}

int start(const Sockets& socks, CALLBACK callback)
{
    fd_set fdset;
    SOCKET sockMax = socks.sock;
    while (g_status) {
        FD_ZERO(&fdset);
        FD_SET(socks.sock, &fdset);
        timeval timeout = { 0, 3000 };
        int sel = select(socks.sock + 1, &fdset, nullptr, nullptr, &timeout);
        if (sel > 0 && FD_ISSET(socks.sock, &fdset)) {
            struct sockaddr_in client_addr { };
            socklen_t len = sizeof(client_addr);
            SOCKET sockNew = accept(socks.sock, reinterpret_cast<sockaddr*>(&client_addr), &len);
            if (sockNew == INVALID_SOCKET) {
                perror("accept failed");
                continue;
            }
            char addr[INET_ADDRSTRLEN];
            printf("accept client %s:%d.\n",
                inet_ntop(AF_INET, &client_addr.sin_addr, addr, sizeof(addr)),
                ntohs(client_addr.sin_port));
#ifdef ON_BIO
            long item = 0;
            item++;
            char __data[8];
            sprintf_s(__data, "%d", item);
            send(sockNew, __data, 8, 0);
            u_long ul = 0;
            int iResult = ioctlsocket(sockNew, FIONBIO, (unsigned long*)&ul);
            printf("ioctl socket status: %d\n", iResult);
#endif
            ssize_t length = BuffSize;
            uint8_t buff[BuffSize];
            auto start_time = std::chrono::high_resolution_clock::now();
            size_t total_bytes = 0;
            do {
                ssize_t size = 0;
                ssize_t last = size;
                memset(buff, 0, BuffSize);
                size = recv(sockNew, buff + size, sizeof(buff), 0);
                if (size == SOCKET_ERROR) {
                    printf("socket recv error: %s!\n", strerror(errno));
                    break;
                } else if (size > 0) {
                    if (callback == nullptr) {
                        callback = g_callback;
                    }
                    if (callback != nullptr) {
                        callback(buff + last, size);
                        length -= size;
                    } else {
                        printf("deal callback is null!\n");
                        return -1;
                    }
                    total_bytes += size;
                    g_rcvdBytes += size;
                } else if (size < 0) {
                    if (errno == EINTR || errno == EWOULDBLOCK) {
                        printf("(slow system call): %u\n", ntohl((u_long)inet_addr(inet_ntoa(socks.local.sin_addr))));
                        continue;
                    } else {
                        printf("connect failed, try again!\n");
                    }
                } else if (size == 0) {
                    printf("client peer closed.\n");
                    break;
                }
            } while (length > 0);
            std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - start_time;
            printf("Total received: %zu bytes in %.2f seconds (%.2f bytes/sec).\n",
                total_bytes, elapsed.count(),
                elapsed.count() > 0 ? total_bytes / elapsed.count() : 0.0);
            close(sockNew);
            sockMax = std::max(sockMax, sockNew);
        }
    }
    printf("sockMax=%d+1\n", sockMax);
    return 0;
}

void finish(const Sockets& socks)
{
    if (socks.sock != INVALID_SOCKET)
        close(socks.sock);
#ifdef SAVE_DATA
    if (g_fld != -1)
        close(g_fld);
#endif
}

int dump(uint8_t* buf, uint32_t len)
{
    int size = len;
#ifdef SAVE_DATA
    size = write(g_fld, buf, len);
#else
    PRINT_RECV(buf, len);
#endif
    return size;
}

int main(int argc, char* argv[])
{
    int port = 9999;
    if (argc > 1) port = atoi(argv[1]);
    Sockets socks = setup(port);
    if (socks.sock != INVALID_SOCKET) {
        start(socks, dump);
        finish(socks);
    }
    return 0;
}
