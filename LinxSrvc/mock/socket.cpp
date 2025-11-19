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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

struct RuntimeState {
    float fBps = 0;
    float progress = 0;
    bool bserv = false;
    bool running = true;
    bool dealFile = false;
    bool bytcp = false;
    FILE* filep = NULL;
    int fileno = -1;
    int sock = -1;
    int port = 8899;
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

mutex g_mutex{};
queue<Message*> g_msgQue{};
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

void signal_exit(int s)
{
    if (g_state.dealFile && g_state.filep != NULL) {
        fclose(g_state.filep);
        g_state.filep = NULL;
    }
    if (g_state.running) {
        g_state.running = false;
    }
    if (g_state.sock != -1) {
        close(g_state.sock);
        g_state.sock = -1;
    }
    cout << "ctrl-c, socket close, SIG=" << s << "." << endl;
    exit(0);
}

int server(int argc, char* argv[])
{
    const char* file = "./test.dat";
    g_state.bserv = true;
    if (argc > 1) {
        g_state.port = atoi(argv[1]);
    } else {
        cout << "Usage: " << argv[0] << " <listen port> <TCP(1)/UDP(0)> [pkgsize(default 1024)] [filename]" << endl;
        return -1;
    }
    if (argc > 2) {
        g_state.bytcp = atoi(argv[2]);
    } else {
        cout << "Usage: " << argv[0] << " <listen port> <TCP(1)/UDP(0)> [pkgsize(default 1024)] [save filename]" << endl;
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
        g_state.dealFile = true;
        file = argv[4];
    }
    int ssock = -1;
    if (g_state.bytcp) {
        if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) == ~0) {
            perror("socket");
            return -4;
        }
    } else {
        ssock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    g_state.sock = ssock;
    cout << "server start by " << (g_state.bytcp ? "TCP" : "UDP") << ", pkgsize:" << pkgsize <<
        (g_state.dealFile ? ", write to '" + string(file) + "'" : "") << " ok." << endl;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(g_state.port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(ssock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        close(ssock);
        perror("bind");
        return -5;
    }
    cout << "socket bind port " << g_state.port << " ok." << endl;
    if (g_state.bytcp) {
        if (listen(ssock, 50) < 0) {
            close(ssock);
            perror("listen");
            return -6;
        }
    }
    cout << "socket listen INADDR_ANY(" << ssock << ")." << endl;
    uint64_t start = getUsecTime();
    uint64_t total = 0;
    uint64_t calclen = 0;
    timeval timeout = { 0, 3000 };
    socklen_t locsize = sizeof(local);
    unsigned char msgbuf[pkgsize];
    char ip[16];
    fd_set fds;
    FD_ZERO(&fds);
    while (g_state.running) {
        if (g_state.bytcp) {
            int csock = accept(ssock, (struct sockaddr*)&local, &locsize);
            if (csock < 0) {
                close(csock);
                perror("accept");
                return -7;
            }
            if (g_state.dealFile) {
                if ((g_state.filep = fopen(file, "wb+")) == NULL) {
                    fprintf(stderr, "recv fopen error: %s.\n", strerror(errno));
                    return -1;
                }
            }
            inet_ntop(AF_INET, (void*)&local.sin_addr, ip, 16);
            cout << "socket accept from " << ip << ":" << ntohs(local.sin_port) << ", waiting message..." << endl;
            while (true) {
                FD_SET(csock, &fds);
                if (select((int)(csock + 1), &fds, NULL, NULL, &timeout) > 0) {
                    if (FD_ISSET(csock, &fds) > 0) {
                        ssize_t rcvlen = ::recv(csock, (char*)msgbuf, pkgsize, 0);
                        if (rcvlen > 0) {
                            calclen += rcvlen;
                            total += rcvlen;
                            if (getUsecTime() - start > 1000000ULL) {
                                g_state.fBps = (calclen * 1.f) / (getUsecTime() - start) * 1000000.f;
                                start = getUsecTime();
                                calclen = 0;
                            }
                            if (g_state.dealFile && g_state.filep != NULL) {
                                int i_write_count = fwrite(msgbuf, rcvlen, sizeof(char), g_state.filep);
                                if (i_write_count != sizeof(char)) {
                                    fprintf(stderr, "recv data write failed: %s, write(count=%d,size=%zd).\n", strerror(errno), i_write_count, rcvlen);
                                    fclose(g_state.filep);
                                    g_state.filep = NULL;
                                }
                                if (rcvlen < 0x10000) {
                                    fsync(fileno(g_state.filep));
                                }
                            }
                        } else if (rcvlen == 0) {
                            sync();
                            fclose(g_state.filep);
                            g_state.filep = NULL;
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
                }
            }
        } else {
            FD_SET(ssock, &fds);
            if (select((int)(ssock + 1), &fds, NULL, NULL, &timeout) > 0) {
                if (FD_ISSET(ssock, &fds) > 0) {
                    ssize_t rcvlen = 0;
                    if ((rcvlen = ::recvfrom(ssock, (char*)msgbuf, pkgsize, 0, (struct sockaddr*)&local, &locsize)) < 0) {
                        continue;
                    }
                    calclen += rcvlen;
                    if (getUsecTime() - start > 1000000ULL) {
                        g_state.fBps = (calclen * 1.f) / (getUsecTime() - start) * 1000000.f;
                        start = getUsecTime();
                        calclen = 0;
                    }
                    for (int i = 0; i < rcvlen; i++) {
                        if ((i % 32 == 0) && (i > 0))
                            printf("\n");
                        printf("%02x ", msgbuf[i]);
                    }
                    printf("\nrecv[%d] size %zd ok.\n", ssock, rcvlen);
                }
            }
        }
    }
    signal_exit(0);
    cout << "server exit." << endl;
    return 0;
}

void usage(const char* prog)
{
    cout << "Usage: " << prog << " <IP> <server port> [TCP(1)/UDP(0, default)] [package size(64B, default)] [filename] [thread count]" << endl;
}

int client(int argc, char* argv[])
{
    int ssock = -1;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    const char* ip = "192.168.197.140";
    int caplen = 1024;
    const char* file = "./test";
    size_t thrds = 1;
    g_state.bserv = false;
    vector<bool> vecstat(0);
    char msgbuf[caplen];
    if (argc > 1) {
        ip = argv[1];
    } else {
        usage(argv[0]);
        return -1;
    }
    local.sin_addr.s_addr = inet_addr(ip);
    if (argc > 2) {
        g_state.port = atoi(argv[2]);
    } else {
        usage(argv[0]);
        return -1;
    }
    local.sin_port = htons(g_state.port);
    if (argc > 3) {
        g_state.bytcp = atoi(argv[3]);
    }
    if (argc > 4) {
        caplen = atoi(argv[4]);
        if (caplen > 0x400000) {
            caplen = 0x400000;
            cout << "message caplen too big, fixed to 4M." << endl;
        }
    }
    if (argc > 5) {
        file = argv[5];
        g_state.dealFile = true;
    }
    cout << "client start by " << (g_state.bytcp ? "TCP" : "UDP") << " send-to " << ip << ":" << g_state.port << " caplen=" << caplen << (g_state.dealFile ? ", file=" + string(file) : "") << " ok." << endl;
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
    if (g_state.bytcp) {
        ssock = socket(PF_INET, SOCK_STREAM, 0);
        if (ssock == -1) {
            perror("socket");
            return -1;
        }
        int ret = connect(ssock, (struct sockaddr*)&local, sizeof(local));
        if (ret == -1) {
            perror("connect");
            return -1;
        }
    } else {
        ssock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    if (argc <= 5) {
        cout << "type message to send:" << endl;
        while (cin >> msgbuf) {
            int len = strnlen(msgbuf, sizeof(msgbuf) - 1) + 1;
            msgbuf[sizeof(msgbuf) - 1] = '\0'; // Ensure null-termination
            if (g_state.bytcp) {
                int bytes = send(ssock, (const char*)msgbuf, len, 0);
                if (bytes < 0) {
                    perror("send");
                    continue;
                }
            } else {
                ::sendto(ssock, (const char*)msgbuf, len, 0, (struct sockaddr*)&local, sizeof(local));
            }
            cout << "sent [" << msgbuf << "] to " << ip << endl;
        }
    } else {
        g_state.fileno = open(file, O_RDONLY, 0666);
        if (g_state.fileno != -1) {
            uint64_t sentLen = 0;
            long total = lseek(g_state.fileno, 0, SEEK_END);
            lseek(g_state.fileno, 0, SEEK_SET);
            uint64_t start = getUsecTime();
            uint64_t current = start;
            uint64_t calcsize = 0;
            ssize_t rdsize = 0;
            ssize_t msglen = sizeof(msgbuf) > caplen ? caplen : sizeof(msgbuf);
            while ((rdsize = read(g_state.fileno, msgbuf, msglen)) > 0) {
                if (rdsize != msglen) {
                    fprintf(stdout, "read last size=%zd, expect=%zd: error: %s\n", rdsize, msglen, strerror(errno));
                }
                if (g_state.bytcp) {
                    if (thrds > 1) {
                        Message msg;
                        msg.sock = ssock;
                        msg.size = rdsize;
                        msg.addr = msgbuf;
                        do {
                            if (queueSize() < MaxQueueSize) {
                                queuePush(&msg);
                                break;
                            } else {
                                // fprintf(stderr, "queue size = %ld is full waiting free...\n", queueSize());
                            }
                        } while (true);
                    } else {
                        int bytes = send(ssock, (const char*)msgbuf, rdsize, 0);
                        if (bytes < 0) {
                            perror("send");
                            continue;
                        }
                        sentLen += bytes;
                        calcsize += bytes;
                        if (getUsecTime() - current > 1000000ULL) {
                            g_state.fBps = (calcsize * 1.f) / (getUsecTime() - current) * 1000000.f;
                            g_state.progress = sentLen * 1.00f / total;
                            current = getUsecTime();
                            calcsize = 0;
                        }
                    }
                } else {
                    sentLen += ::sendto(ssock, (const char*)msgbuf, rdsize, 0, (struct sockaddr*)&local, sizeof(local));
                }
            }
            while (g_state.bytcp && thrds > 1) {
                bool status = true;
                for (size_t i = 0; i < vecstat.size(); i++) {
                    status &= vecstat[i];
                }
                if (status) {
                    vecstat.clear();
                    break;
                }
            }
            fprintf(stdout, "sent %.3fM over, average speed is %.3f MB/s\n", sentLen * 1.0f / 1048576, (sentLen * 1.f) / (getUsecTime() - start) * 1048576 / 1000000.f);
            close(g_state.fileno);
        } else {
            perror("open");
        }
    }
    close(ssock);
    return 0;
}

int main(int argc, char* argv[])
{
    thread task(
        [&]() -> void {
            float lastValue = 0;
            string lastUnit = "";
            while (g_state.running) {
                float value = 0;
                string unit = "B/s";
                if (g_state.fBps >= 1000000 && ((int)g_state.fBps) % 1000000 > 0) {
                    value = g_state.fBps / 1000000.f;
                    unit = "MB/s";
                } else if (g_state.fBps >= 1000 && ((int)g_state.fBps) % 1000 > 0) {
                    value = g_state.fBps / 1000.f;
                    unit = "KB/s";
                } else {
                    value = g_state.fBps;
                }
                if (g_state.bserv) {
                    if (lastValue != value || lastUnit != unit) {
                        fprintf(stdout, "recvd speed %.3f %s\r", value, unit.c_str());
                        fflush(stdout);
                        lastValue = value;
                        lastUnit = unit;
                    }
                } else {
                    if (lastValue != value || lastUnit != unit) {
                        fprintf(stdout, "sent %3.3f%% speed %.3f %s\r", g_state.progress * 100, value, unit.c_str());
                        fflush(stdout);
                        lastValue = value;
                        lastUnit = unit;
                    }
                }
                wait(10);
            }
        });
    if (task.joinable()) {
        task.detach();
    }
    signal(SIGINT, signal_exit);
    int status = 0;
#ifndef CLIENT
    status = server(argc, argv);
#else
    status = client(argc, argv);
#endif
    g_state.running = false;
    return status;
}
