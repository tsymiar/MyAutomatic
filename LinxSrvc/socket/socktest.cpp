#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

struct RuntimeState {
    float fMBps = 0;
    float fKBps = 0;
    float progress = 0;
    bool server = false;
    bool running = true;
    bool save2file = false;
} g_state;

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

void wait(unsigned int tms)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000 * tms; // 10 milliseconds
    nanosleep(&ts, NULL);
}

int server(int argc, char* argv[])
{
    g_state.server = true;
    int port = 8899;
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        cout << "Usage: " << argv[0] << " <listen port> <TCP(1)/UDP(0)> <pkgsize(default 1024)> [filename]" << endl;
        return -1;
    }
    bool bytcp = false;
    if (argc > 2) {
        bytcp = atoi(argv[2]);
    } else {
        cout << "Usage: " << argv[0] << " <listen port> <TCP(1)/UDP(0)> <pkgsize(default 1024)> [save filename]" << endl;
        return -1;
    }
    int pkgsize = 1024;
    if (argc > 3) {
        pkgsize = atoi(argv[3]);
        if (pkgsize > 0x400000) {
            pkgsize = 0x400000;
            cout << "message size too big, fixed to 4M." << endl;
        }
    }
    if (argc > 4) {
        g_state.save2file = true;
    }
    int vsock = -1;
    if (bytcp) {
        if ((vsock = socket(AF_INET, SOCK_STREAM, 0)) == ~0) {
            perror("socket");
            return -4;
        }
    } else {
        vsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    cout << "socket create by " << (bytcp ? "tcp" : "udp") << " ok." << endl;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(vsock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        close(vsock);
        perror("bind");
        return -5;
    }
    cout << "socket bind port " << port << " ok." << endl;
    if (bytcp) {
        if (listen(vsock, 50) < 0) {
            close(vsock);
            perror("listen");
            return -6;
        }
    }
    cout << "socket listen INADDR_ANY(" << vsock << ")." << endl;
    uint64_t start = getUsecTime();
    uint64_t total = 0;
    fd_set fds;
    FD_ZERO(&fds);
    uint64_t curlen = 0;
    timeval timeout = { 0, 3000 };
    socklen_t locsize = sizeof(local);
    unsigned char message[pkgsize];
    FILE* pfile = NULL;
    if (g_state.save2file) {
        if ((pfile = fopen("./test.dat", "wb+")) == NULL) {
            fprintf(stderr, "recv fopen error: %s.\n", strerror(errno));
            return -1;
        }
    }
    while (g_state.running) {
        if (bytcp) {
            int csock = accept(vsock, (struct sockaddr*)&local, &locsize);
            if (csock < 0) {
                close(csock);
                perror("accept");
                return -7;
            }
            char addr[16];
            inet_ntop(AF_INET, (void*)&local.sin_addr, addr, 16);
            cout << "socket accept from " << addr << ":" << ntohs(local.sin_port) << ", waiting message..." << endl;
            while (true) {
                int rcvlen = ::recv(csock, (char*)message, pkgsize, 0);
                if (rcvlen > 0) {
                    curlen += rcvlen;
                    total += rcvlen;
                    if (getUsecTime() - start > 1000000ULL) {
                        g_state.fKBps = (curlen * 1.f) / (getUsecTime() - start) * 1048576 / 1000.f;
                        start = getUsecTime();
                        curlen = 0;
                    }
                    // TODO something to catch the message message
                } else if (rcvlen == 0) {
                    cout << "\nrcvd total size: " << total << endl;
                    close(csock);
                    cout << "lose connection(" << csock << ")" << endl;
                    total = 0;
                    break;
                } else {
                    close(csock);
                    perror("recv");
                    break;
                }
            }
        } else {
            FD_SET(vsock, &fds);
            if (select((int)(vsock + 1), &fds, NULL, NULL, &timeout) > 0) {
                if (FD_ISSET(vsock, &fds) > 0) {
                    ssize_t rcvlen = 0;
                    if ((rcvlen = ::recvfrom(vsock, (char*)message, pkgsize, 0, (struct sockaddr*)&local, &locsize)) < 0) {
                        continue;
                    }
                    curlen += rcvlen;
                    if (getUsecTime() - start > 1000000ULL) {
                        g_state.fKBps = (curlen * 1.f) / (getUsecTime() - start) * 1048576 / 1000.f;
                        start = getUsecTime();
                        curlen = 0;
                    }
                    if (g_state.save2file && pfile != NULL) {
                        int i_write_count = fwrite(message, sizeof(char), pkgsize, pfile);
                        if (i_write_count != pkgsize) {
                            fprintf(stderr, "recv fwrite failed(write_count=%d,reallen=%d).\n", i_write_count, pkgsize);
                            fclose(pfile);
                            pfile = NULL;
                        }
                    } else {
                        for (int i = 0; i < rcvlen; i++) {
                            if ((i % 32 == 0) && (i > 0))
                                printf("\n");
                            printf("%02x ", message[i]);
                        }
                        printf("\nrecvfrom [%d] size %ld ok.\n", vsock, rcvlen);
                    }
                }
            }
        }
    }
    if (g_state.save2file && pfile != NULL) {
        fclose(pfile);
        pfile = NULL;
    }
    if (vsock > 0)
        close(vsock);
    cout << "server exit." << endl;
    return 0;
}

void usage(const char* prog)
{
    cout << "Usage: " << prog << " <IP> <server port> [TCP(1)/UDP(0, default)] [package size(64B, default)] [save filename] [thread count]" << endl;
}

int client(int argc, char* argv[])
{
    int vsock = -1;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    const char* ip = "192.168.197.140";
    int port = 8899;
    int size = 1024;
    const char* file = "./test";
    bool bytcp = false;
    size_t thrds = 1;
    vector<bool> vecstat(0);
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
        bytcp = atoi(argv[3]);
    }
    if (argc > 4) {
        size = atoi(argv[4]);
        if (size > 0x400000) {
            size = 0x400000;
            cout << "message size too big, fixed to 4M." << endl;
        }
    }
    if (argc > 6) {
        thrds = atoi(argv[6]);
        thread works[thrds];
        vecstat.resize(thrds);
        for (size_t i = 0; i < vecstat.size(); i++) {
            vecstat[i] = false;
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
                                wait(10);
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
                vecstat[i] = true;
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
        vsock = socket(PF_INET, SOCK_STREAM, 0);
        if (vsock == -1) {
            perror("socket");
            return -1;
        }
        int ret = connect(vsock, (struct sockaddr*)&local, sizeof(local));
        if (ret == -1) {
            perror("connect");
            return -1;
        }
    } else {
        vsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    char message[size];
    if (argc < 5) {
        cout << "type message to send:" << endl;
        while (cin >> message) {
            int len = strnlen(message, sizeof(message) - 1) + 1;
            message[sizeof(message) - 1] = '\0'; // Ensure null-termination
            if (bytcp) {
                ::sendto(vsock, (const char*)message, len, 0, (struct sockaddr*)&local, sizeof(local));
            } else {
                int bytes = send(vsock, (const char*)message, len, 0);
                if (bytes < 0) {
                    perror("send");
                    continue;
                }
            }
            cout << "sent [" << message << "] to " << ip << endl;
        }
    } else {
        file = argv[5];
        cout << "client start send-to " << ip << ":" << port << " message=" << size << ", file=" << file << (bytcp ? ", sending by TCP." : ", send by UDP.") << endl;
        FILE* fp = fopen(file, "rb");
        if (fp != NULL) {
            uint64_t sentLen = 0;
            fseek(fp, 0, SEEK_END);
            long total = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            uint64_t begin = getUsecTime();
            uint64_t current = begin;
            uint64_t size = 0;
            g_state.server = false;
            while (!feof(fp)) {
                size_t nrSize = fread(message, 1, sizeof(message), fp);
                if (nrSize != sizeof(message)) {
                    fprintf(stdout, "read last size=%zu, expect=%zu: %s\n", nrSize, sizeof(message), strerror(errno));
                }
                if (bytcp) {
                    if (thrds > 1) {
                        Message msg;
                        msg.sock = vsock;
                        msg.addr = message;
                        msg.size = nrSize;
                        do {
                            if (queueSize() < MaxQueueSize) {
                                queuePush(&msg);
                                break;
                            } else {
                                // fprintf(stderr, "queue size = %ld is full waiting free...\n", queueSize());
                            }
                        } while (true);
                    } else {
                        int bytes = send(vsock, (const char*)message, nrSize, 0);
                        if (bytes < 0) {
                            perror("send");
                            continue;
                        }
                        sentLen += bytes;
                        size += bytes;
                        if (getUsecTime() - current > 1000000ULL) {
                            g_state.fMBps = (size * 1.f) / (getUsecTime() - current) * 1048576 / 1000000.f;
                            g_state.progress = sentLen * 1.00f / total;
                            current = getUsecTime();
                            size = 0;
                        }
                    }
                } else {
                    sentLen += ::sendto(vsock, (const char*)message, nrSize, 0, (struct sockaddr*)&local, sizeof(local));
                }
            }
            while (bytcp && thrds > 1) {
                bool status = true;
                for (size_t i = 0; i < vecstat.size(); i++) {
                    status &= vecstat[i];
                }
                if (status) {
                    vecstat.clear();
                    break;
                }
            }
            fprintf(stdout, "send over, average speed is %.3f MB/s\n", (sentLen * 1.f) / (getUsecTime() - begin) * 1048576 / 1000000.f);
            close(fileno(fp));
        } else {
            perror("open");
        }
    }
    close(vsock);
    return 0;
}

int main(int argc, char* argv[])
{
    thread task(
        [&]() -> void {
            float bps = 0;
            while (g_state.running) {
                if (g_state.server) {
                    if (bps != g_state.fKBps) {
                        fprintf(stdout, "recv speed %.3f KB/s\r", g_state.fKBps);
                        fflush(stdout);
                        bps = g_state.fKBps;
                    }
                } else {
                    if (bps != g_state.fMBps) {
                        fprintf(stdout, "send %3.3f%% speed %.3f MB/s\r", g_state.progress * 100, g_state.fMBps);
                        fflush(stdout);
                        bps = g_state.fMBps;
                    }
                }
                wait(10);
            }
        });
    if (task.joinable()) {
        task.detach();
    }
    int status = 0;
#ifndef CLIENT
    status = server(argc, argv);
#else
    status = client(argc, argv);
#endif
    g_state.running = false;
    return status;
}
