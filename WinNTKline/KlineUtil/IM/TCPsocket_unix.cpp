#include <stdio.h> // printf
#include <errno.h> // errno
#include <fcntl.h> // O_CREAT
#include <stdint.h> // uint8_t
#include <stdlib.h> // atoi
#include <unistd.h> // usleep
#include <sys/socket.h> // AF_INET
#include <netinet/in.h> // sockaddr_in
#include <string.h> // strerror
#include <arpa/inet.h> // inet_ntoa

typedef int (*CALLBACK) (uint8_t*, uint32_t);
typedef int SOCKET;
#define nullptr NULL
#define INVALID_SOCKET (~0)
#define SOCKET_ERROR (-1)
#define PRINT_RECV(txt, len) do { \
    fprintf(stdout, "----[ "); \
    for (int c = 0; c < len; c++) { \
        if (c > 0 && c % 32 == 0) \
            fprintf(stdout, "\n"); \
        fprintf(stdout, "%02x ", static_cast<unsigned char>(txt[c])); \
    } \
    fprintf(stdout, "]----\n"); \
} while (0);
// #define SAVE_DATA

struct Sockets {
    SOCKET sock;
    struct sockaddr_in local;
};

CALLBACK g_callback = nullptr;
const int BuffSize = 1024;
static int g_fld = -1;
bool g_status = false;

Sockets setup(short port);

void RegisterCallback(CALLBACK CALLBACK);

int start(const Sockets& sock, CALLBACK CALLBACK = nullptr);

void finish(const Sockets&);

static int dump(uint8_t*, uint32_t);

void sigHandle(int s)
{
    g_status = false;
    printf("exit!\n");
}

int main(int argc, char* argv[])
{
    int port = 9999;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    Sockets socks = setup(port);
    start(socks, dump);
    finish(socks);
    return 0;
}

Sockets setup(short port)
{
    Sockets socks{};
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("invalid socket!\n");
        g_status = false;
        return socks;
    }
    printf("begin socket setup\n");
    struct sockaddr_in local;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    printf("start bind %d\n", port);
    if (bind(sock, (sockaddr*)&local, sizeof(local)) != 0) {
        close(sock);
        printf("binds port[%d] failed: %s!\n", port, strerror(errno));
        g_status = false;
        return socks;
    }
    printf("listening...\n");
    if (listen(sock, 5) != 0) {
        close(sock);
        printf("listen port[%d] failed: %s!\n", port, strerror(errno));
        g_status = false;
        return socks;
    }
#ifdef SAVE_DATA
    g_fld = open("dump.bin", O_CREAT | O_RDWR | O_APPEND, 0644);
#endif
    socks.sock = sock;
    socks.local = local;
    g_status = true;

    signal(SIGINT, sigHandle);

    return socks;
}

void RegisterCallback(CALLBACK callback)
{
    g_callback = callback;
}

int start(const Sockets& socks, CALLBACK callback)
{
    fd_set fdset;
    FD_ZERO(&fdset);
    SOCKET sockMax = socks.sock;
    while (g_status) {
        FD_SET(socks.sock, &fdset);
        timeval timeout = { 0, 3000 };
        if (select((int)(socks.sock + 1), &fdset, NULL, NULL, &timeout) > 0) {
            SOCKET sockNew;
            if (FD_ISSET(socks.sock, &fdset) > 0) {
                socklen_t len = sizeof(socks.local);
                sockNew = accept(socks.sock, (sockaddr*)&socks.local, &len);
                char addr[INET_ADDRSTRLEN];
                struct sockaddr_in peer { };
                getpeername(sockNew, reinterpret_cast<struct sockaddr*>(&peer), &len);
                printf("accept client %s:%d.\n", inet_ntop(AF_INET, &peer.sin_addr, addr, sizeof(addr)), ntohs(peer.sin_port));
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
                int length = BuffSize;
                uint8_t buff[BuffSize];
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
            }
            sockMax = (sockMax > (int)sockNew ? sockMax : sockNew);
        }
    }
    printf("sockMax=%d+1\n", sockMax);
    return 0;
}

void finish(const Sockets& socks)
{
    if (socks.sock > 0)
        close(socks.sock);
#ifdef SAVE_DATA
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
