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
CALLBACK g_callback = nullptr;

struct Sockets {
    bool runs;
    SOCKET sock;
    struct sockaddr_in local;
};

Sockets init(short port);

void RegisterCallback(CALLBACK CALLBACK);

int start(Sockets sock, CALLBACK CALLBACK = nullptr);

void finish(Sockets);

static int dump(uint8_t*, uint32_t);

int main(int argc, char* argv[])
{
    int port = 9999;
    if (argc > 1)
        port = atoi(argv[1]);
    Sockets socks = init(port);
    start(socks, dump);
    usleep(10000000);
    finish(socks);
    return 0;
}

Sockets init(short port)
{
    Sockets socks;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("invalid socket!\n");
        socks.runs = false;
        return socks;
    }
    printf("socket initting\n");
    struct sockaddr_in local;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    printf("start bind %d\n", port);
    if (bind(sock, (sockaddr*)&local, sizeof(local)) != 0) {
        close(sock);
        printf("binds port[%d] failed: %s!", port, strerror(errno));
        socks.runs = false;
        return socks;
    }
    printf("listening...\n");
    if (listen(sock, 5) != 0) {
        close(sock);
        printf("listen port[%d] failed: %s!", port, strerror(errno));
        socks.runs = false;
        return socks;
    }
    socks.sock = sock;
    socks.local = local;
    socks.runs = true;
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
    while (socks.runs) {
        SOCKET sockNew = 0;
        SOCKET sockMax = socks.sock;
        FD_SET(socks.sock, &fds);
        timeval timeout = { 0, 3000 };
        if (select((int)(sockMax + 1), &fds, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(socks.sock, &fds) > 0) {
                socklen_t len = sizeof(socks.local);
                sockNew = accept(socks.sock, (sockaddr*)&socks.local, &len);
            #if 0
                long item = 0;
                item++;
                char __data[8];
                sprintf_s(__data, "%d", item);
                send(sockNew, __data, 8, 0);
                u_long ul = 0;
                int iResult = ioctlsocket(sockNew, FIONBIO, (unsigned long*)&ul);
            #endif
                int size = 0;
                uint8_t buff[1024];
                do {
                    memset(buff, 0, 1024);
                    size = recv(sockNew, buff, sizeof(buff), 0);
                    if (size == SOCKET_ERROR) {
                        printf("socket error: %s!", strerror(errno));
                        continue;
                    } else if (size > 0) {
                        if (callback == nullptr) {
                            callback = g_callback;
                        }
                        if (callback != nullptr) {
                            callback(buff, size);
                        } else {
                            printf("deal callback is null!");
                            return -1;
                        }
                    } else if (size < 0) {
                        if (errno == EINTR || errno == EWOULDBLOCK) {
                            printf("(slow system call): %u\n", ntohl((u_long)inet_addr(inet_ntoa(socks.local.sin_addr))));
                            continue;
                        } else {
                            printf("connect failed, try again!");
                        }
                    } else if (size == 0) {
                        printf("client peer closed.");
                    }
                } while (size > 0);
            }
            sockMax = (sockMax > (int)sockNew ? sockMax : sockNew);
        }
    }
    return 0;
}

void finish(Sockets socks)
{
    this->socks.runs = false;
    close(socks.sock);
}

int dump(uint8_t* buf, uint32_t len)
{
    int fd = open("test", O_CREAT | O_RDWR);
    write(fd, buf, len);
    close(fd);
    return 0;
}
