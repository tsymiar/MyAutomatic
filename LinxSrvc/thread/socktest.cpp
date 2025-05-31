#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <net/if.h>
#include <sys/time.h>
#include <pthread.h>
#include <thread>
#include <getopt.h>
#include <errno.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <atomic>

using namespace std;

struct Runtime {
    float fMBps = 0;
    float fKBps = 0;
    float progress = 0;
    bool server = false;
    bool running = true;
} g_runtime;

struct Message {
    char* addr;
    int size;
    int sock;
    Message()
    {
        sock = -1;
        size = 0;
        addr = nullptr;
    }
};

mutex g_mutex;
queue<Message*> g_msgQue;
const int MaxQueueSize = 1000;
FILE* g_pfd = NULL;

uint64_t getUsecTime()
{
    uint64_t usec = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    usec = tv.tv_sec * 1000000ULL + tv.tv_usec;
    return usec;
}

void queuePush(Message* msg)
{
    lock_guard<mutex> guard(g_mutex);
    g_msgQue.push(msg);
}

int queuePop(Message* msg)
{
    lock_guard<mutex> guard(g_mutex);
    Message* m = g_msgQue.front();
    if (g_msgQue.size() > 0 && msg != nullptr) {
        *msg = *m;
        g_msgQue.pop();
        return 0;
    } else {
        if (g_msgQue.size() > 0) {
            g_msgQue.pop();
        }
        return -1;
    }
}

size_t queueSize()
{
    return g_msgQue.size();
}

int server(int argc, char* argv[])
{
    int port = 8899;
    bool bytcp = false;
    int pkgsize = 1024;
    g_runtime.server = true;
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        cout << "Usage: " << argv[0] << " <port> <TCP(1)/UDP(0)> <pkgsize(default 1024)>" << endl;
        return -1;
    }
    if (argc > 2) {
        bytcp = atoi(argv[2]);
    } else {
        cout << "Usage: " << argv[0] << " <port> <TCP(1)/UDP(0)> <pkgsize(default 1024)>" << endl;
        return -2;
    }
    if (argc > 3) {
        pkgsize = atoi(argv[3]);
        if (pkgsize > 0x400000) {
            pkgsize = 0x400000;
            cout << "message size too big, fixed to 4M." << endl;
        }
    }
    int _sock = -1;
    if (bytcp) {
        if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) == ~0) {
            perror("socket");
            return -4;
        }
    } else {
        _sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    cout << "socket create ok" << endl;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(_sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        close(_sock);
        perror("bind");
        return -5;
    }
    cout << "socket bind ok" << endl;
    if (bytcp) {
        if (listen(_sock, 50) < 0) {
            close(_sock);
            perror("listen");
            return -6;
        }
    }
    cout << "socket listen INADDR_ANY " << port << endl;

    uint64_t start = getUsecTime();
    uint64_t total = 0;
    fd_set fds;
    FD_ZERO(&fds);
    timeval timeout = { 0, 3000 };
    uint64_t currLen = 0;
    socklen_t locSize = sizeof(local);
    unsigned char message[pkgsize];
    if (g_pfd == NULL) {
        if ((g_pfd = fopen("/mnt/1/test.dat", "wb+")) == NULL) {
            fprintf(stderr, "recv fopen err=%s.", strerror(errno));
            return -1;
        }
    }

    while (true) {
        if (bytcp) {
            int csock = accept(_sock, (struct sockaddr*)&local, &locSize);
            if (csock < 0) {
                close(csock);
                perror("accept");
                return -7;
            }
            char addr[16];
            inet_ntop(AF_INET, (void*)&local.sin_addr, addr, 16);
            cout << "socket accept from " << addr << ":" << ntohs(local.sin_port) << endl;
            while (true) {
                int rcvLen = ::recv(csock, (char*)message, pkgsize, 0);
                if (rcvLen > 0) {
                    currLen += rcvLen;
                    total += rcvLen;
                    if (getUsecTime() - start > 1000000ULL) {
                        g_runtime.fKBps = (currLen * 1.f) / (getUsecTime() - start) * 1048576 / 1000.f;
                        start = getUsecTime();
                        currLen = 0;
                    }
                    // TODO something to catch the message message
                } else if (rcvLen == 0) {
                    cout << "\ntotal size: "
                        << total << endl;
                    close(csock);
                    cout << "lose connection" << endl;
                    total = 0;
                    break;
                } else {
                    close(csock);
                    perror("recv");
                    break;
                }
            }
        } else {
            FD_SET(_sock, &fds);
            if (select((int)(_sock + 1), &fds, NULL, NULL, &timeout) > 0) {
                if (FD_ISSET(_sock, &fds) > 0) {
                    ssize_t rcvLen = 0;
                    if ((rcvLen = ::recvfrom(_sock, (char*)message, pkgsize, 0, (struct sockaddr*)&local, &locSize)) < 0) {
                        continue;
                    }
                    currLen += rcvLen;
                    if (getUsecTime() - start > 1000000ULL) {
                        g_runtime.fKBps = (currLen * 1.f) / (getUsecTime() - start) * 1048576 / 1000.f;
                        start = getUsecTime();
                        currLen = 0;
                    }
#if 0
                    for (int i = 0; i < rcvLen; i++) {
                        if ((i % 32 == 0) && (i > 0))
                            printf("\n");
                        printf("%02x ", message[i]);
                    }
                    printf("\nrecvfrom [%d] size %ld ok.\n", _sock, rcvLen);
#else
                    int i_write_count = fwrite(message, sizeof(char), pkgsize, g_pfd);
                    if (i_write_count != pkgsize) {
                        fprintf(stderr, "recv fwrite failed(write_count=%d,realLen=%d).", i_write_count, pkgsize);
                        fclose(g_pfd);
                        g_pfd = NULL;
                    }
#endif
                }
            }
        }
    }
    return 0;
}

void usage(const char* prog)
{
    cout << "Usage: " << prog << " <ip> <port> <package size> <filename> <TCP(1)/UDP(0)> [thread count]" << endl;
}

int client(int argc, char* argv[])
{
    int _sock = -1;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    const char* ip = "192.168.197.140";
    int port = 8899;
    int size = 64;
    const char* file = "./test";
    bool bytcp = false;
    size_t thrds = 1;
    vector<bool> vecStat(0);
    if (argc > 1) {
        ip = argv[1];
    } else {
        usage(argv[0]);
        return -1;
    }
    local.sin_addr.s_addr = inet_addr(ip);
    if (argc > 2) {
        port = atoi(argv[2]);
    } else {
        usage(argv[0]);
        return -1;
    }
    local.sin_port = htons(port);
    if (argc > 3) {
        size = atoi(argv[3]);
        if (size > 0x400000) {
            size = 0x400000;
            cout << "message size too big, fixed to 4M." << endl;
        }
    } else {
        usage(argv[0]);
        return -1;
    }
    if (argc > 4) {
        bytcp = atoi(argv[4]);
    } else {
        usage(argv[0]);
        return -1;
    }
    if (argc > 5) {
        file = argv[5];
    } else {
        usage(argv[0]);
        return -1;
    }
    if (argc > 6) {
        thrds = atoi(argv[6]);
        thread works[thrds];
        vecStat.resize(thrds);
        for (size_t i = 0; i < vecStat.size(); i++) {
            vecStat[i] = false;
        }
        for (size_t i = 0; i < thrds; i++) {
            works[i] = thread([&](int i) -> void {
                Message msg;
                fd_set fds;
                timeval latency = { 0, 120 };
                uint64_t total = 0;
                while (true) {
                    FD_ZERO(&fds);
                    FD_SET(msg.sock, &fds);
                    if (select(msg.sock + 1, NULL, &fds, NULL, &latency) > 0) {
                        if (FD_ISSET(msg.sock, &fds) > 0) {
                            if (queuePop(&msg) < 0) {
                                usleep(10000);
                                continue;
                            }
                            if (msg.sock <= 0 || msg.size <= 0 || msg.addr == nullptr) {
                                continue;
                            }
                            int bytes = send(msg.sock, (const char*)msg.addr, msg.size, 0);
                            if (bytes < 0) {
                                fprintf(stderr, "send failed, socket=%d, size=%d.\n", msg.sock, msg.size);
                                continue;
                            }
                            total += msg.size;
                        }
                    }
                    if (queueSize() == 0) {
                        break;
                    }
                }
                vecStat[i] = true;
                cout << "work thread[" << i << "] total size = " << total << endl;
                },
                i);
            if (works[i].joinable()) {
                works[i].detach();
                cout << "work thread[" << i << "] start" << endl;
            } else {
                cout << "work thread[" << i << "] not able to join main thread!" << endl;
            }
        }
    }
    if (bytcp) {
        _sock = socket(PF_INET, SOCK_STREAM, 0);
        if (_sock == -1) {
            perror("socket");
            return -1;
        }
        int ret = connect(_sock, (struct sockaddr*)&local, sizeof(local));
        if (ret == -1) {
            perror("connect");
            return -1;
        }
    } else {
        _sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    char message[size];
    if (argc < 5) {
        while (cin >> message) {
            int len = strlen(message) + 1;
            if (bytcp) {
                ::sendto(_sock, (const char*)message, len, 0, (struct sockaddr*)&local, sizeof(local));
            } else {
                int bytes = send(_sock, (const char*)message, len, 0);
                if (bytes < 0) {
                    perror("send");
                    continue;
                }
            }
            cout << "sent [" << message << "] to " << ip << endl;
        }
    } else {
        cout << "client start send-to " << ip << ":" << port << " message=" << size << ", file=" << file << (bytcp ? ", send by TCP." : ", send by UDP.") << endl;
        FILE* fp = fopen(file, "rb");
        if (fp != NULL) {
            uint64_t sentLen = 0;
            fseek(fp, 0, SEEK_END);
            long total = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            uint64_t begin = getUsecTime();
            uint64_t current = begin;
            uint64_t size = 0;
            g_runtime.server = false;
            while (!feof(fp)) {
                size_t nrSize = fread(message, 1, sizeof(message), fp);
                if (nrSize != sizeof(message)) {
                    fprintf(stdout, "read last size=%ld, expect=%ld: %s\n", nrSize, sizeof(message), strerror(errno));
                }
                if (bytcp) {
                    if (thrds > 1) {
                        Message msg;
                        msg.sock = _sock;
                        msg.addr = message;
                        msg.size = nrSize;
                        do {
                            if (queueSize() < MaxQueueSize) {
                                queuePush(&msg);
                                break;
                            } else {
                                // fprintf(stderr, "queue size = %ld is full wating free...\n", queueSize());
                            }
                        } while (true);
                    } else {
                        int bytes = send(_sock, (const char*)message, nrSize, 0);
                        if (bytes < 0) {
                            perror("send");
                            continue;
                        }
                        sentLen += bytes;
                        size += bytes;
                        if (getUsecTime() - current > 1000000ULL) {
                            g_runtime.fMBps = (size * 1.f) / (getUsecTime() - current) * 1048576 / 1000000.f;
                            g_runtime.progress = sentLen * 1.00f / total;
                            current = getUsecTime();
                            size = 0;
                        }
                    }
                } else {
                    sentLen += ::sendto(_sock, (const char*)message, nrSize, 0, (struct sockaddr*)&local, sizeof(local));
                }
            }
            while (bytcp && thrds > 1) {
                bool status = true;
                for (size_t i = 0; i < vecStat.size(); i++) {
                    status &= vecStat[i];
                }
                if (status) {
                    vecStat.clear();
                    break;
                }
            }
            fprintf(stdout, "send over, average speed is %.3f MB/s\n", (sentLen * 1.f) / (getUsecTime() - begin) * 1048576 / 1000000.f);
            close(fileno(fp));
        } else {
            perror("open");
        }
    }
    close(_sock);
    return 0;
}

int main(int argc, char* argv[])
{
    thread task(
        [&]() -> void {
            float bps = 0;
            while (true) {
                if (g_runtime.server) {
                    if (bps != g_runtime.fKBps) {
                        fprintf(stdout, "recv speed %.3f KB/s\r", g_runtime.fKBps);
                        fflush(stdout);
                        bps = g_runtime.fKBps;
                    }
                } else {
                    if (bps != g_runtime.fMBps) {
                        fprintf(stdout, "send %3.3f%% speed %.3f MB/s\r", g_runtime.progress * 100, g_runtime.fMBps);
                        fflush(stdout);
                        bps = g_runtime.fMBps;
                    }
                }
                usleep(10000);
            }
        });
    if (task.joinable()) {
        task.detach();
    }
#ifdef CLIENT
    client(argc, argv);
#else
    server(argc, argv);
#endif
    return 0;
}
