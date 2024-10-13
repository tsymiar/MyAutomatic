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
    fprintf(stdout, "--------------------------------" \
                    "--------------------------------\n"); \
    for (int c = 0; c < len; c++) { \
        if (c > 0 && c % 32 == 0) \
            fprintf(stdout, "\n"); \
        fprintf(stdout, "%02x ", static_cast<unsigned char>(txt[c])); \
    } \
    fprintf(stdout, "\n"); \
    fprintf(stdout, "--------------------------------" \
                    "--------------------------------\n"); \
} while (0);
#define SAVE_DATA

CALLBACK g_callback = nullptr;
const int BuffSize = 1024;
static int g_fld = -1;

struct Sockets {
    bool running;
    SOCKET sock;
    struct sockaddr_in local;
};

Sockets setup(short port);

void RegisterCallback(CALLBACK CALLBACK);

int start(Sockets sock, CALLBACK CALLBACK = nullptr);

void finish(Sockets);

static int dump(uint8_t*, uint32_t);

int main(int argc, char* argv[])
{
    int port = 9999;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    Sockets socks = setup(port);
    start(socks, dump);
    usleep(10000000);
    finish(socks);
    return 0;
}

Sockets setup(short port)
{
    Sockets socks{};
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("invalid socket!\n");
        socks.running = false;
        return socks;
    }
    printf("socket setup:\n");
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
        socks.running = false;
        return socks;
    }
    printf("listening...\n");
    if (listen(sock, 5) != 0) {
        close(sock);
        printf("listen port[%d] failed: %s!\n", port, strerror(errno));
        socks.running = false;
        return socks;
    }
#ifdef SAVE_DATA
    g_fld = open("dump.bin", O_CREAT | O_RDWR | O_APPEND, 0644);
#endif
    socks.sock = sock;
    socks.local = local;
    socks.running = true;

    return socks;
}

void RegisterCallback(CALLBACK callback)
{
    g_callback = callback;
}

int start(Sockets socks, CALLBACK callback)
{
    fd_set fds;
    FD_ZERO(&fds);
    while (socks.running) {
        SOCKET sockNew = 0;
        SOCKET sockMax = socks.sock;
        FD_SET(socks.sock, &fds);
        timeval timeout = { 0, 3000 };
        if (select((int)(sockMax + 1), &fds, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(socks.sock, &fds) > 0) {
                socklen_t len = sizeof(socks.local);
                sockNew = accept(socks.sock, (sockaddr*)&socks.local, &len);
#ifdef ON_BIO
                long item = 0;
                item++;
                char __data[8];
                sprintf_s(__data, "%d", item);
                send(sockNew, __data, 8, 0);
                u_long ul = 0;
                int iResult = ioctlsocket(sockNew, FIONBIO, (unsigned long*)&ul);
#endif
                int size = 0;
                uint8_t buff[BuffSize];
                do {
                    memset(buff, 0, BuffSize);
                    size = recv(sockNew, buff, sizeof(buff), 0);
                    if (size == SOCKET_ERROR) {
                        printf("socket error: %s!\n", strerror(errno));
                        continue;
                    } else if (size > 0) {
                        if (callback == nullptr) {
                            callback = g_callback;
                        }
                        if (callback != nullptr) {
                            callback(buff, size);
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
                    }
                } while (size > 0);
            }
            sockMax = (sockMax > (int)sockNew ? sockMax : sockNew);
            printf("select sockMax=%d+1\n", sockMax);
        }
    }
    return 0;
}

void finish(Sockets socks)
{
    socks.running = false;
    close(g_fld);
#ifdef SAVE_DATA
    close(socks.sock);
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
