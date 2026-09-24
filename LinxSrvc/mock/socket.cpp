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
#include <atomic>

using namespace std;

const int MAX_PKG_SIZE = 0x400000;      // max size of one packet / one read: 4M
const int MAX_WORK_THREADS = 64;        // max number of sender threads
const int MAX_QUEUE_SIZE = 1000;        // send queue capacity

enum TCPUDP {
    TCP = 1,
    UDP = 0,
    MCAST = 2
};

struct RuntimeState {
    float fBps = 0;
    float progress = 0;
    bool bserv = false;
    bool running = true;
    bool dealFile = false;
    TCPUDP how = TCP;
    FILE* filep = NULL;
    int fileno = -1;
    int sock = -1;
    int port = 8899;
} g_state;

// Queue element owns its own copy of the payload (deep copied on push) and is
// released by the worker after sending, so caller buffers are never borrowed
struct Message {
    char* addr;
    int size;
    int sock;

    Message() : addr(nullptr), size(0), sock(-1) {}
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    Message(Message&& other) noexcept : addr(other.addr), size(other.size), sock(other.sock)
    {
        other.addr = nullptr;
        other.size = 0;
    }
    Message& operator=(Message&& other) noexcept
    {
        if (this != &other) {
            release();
            addr = other.addr;
            size = other.size;
            sock = other.sock;
            other.addr = nullptr;
            other.size = 0;
        }
        return *this;
    }
    ~Message()
    {
        release();
    }

    void release()
    {
        if (addr != nullptr) {
            free(addr);
            addr = nullptr;
        }
        size = 0;
    }

    bool assign(const void* data, size_t len, int fd)
    {
        release();
        addr = static_cast<char*>(malloc(len > 0 ? len : 1));
        if (addr == nullptr) {
            return false;
        }
        memcpy(addr, data, len);
        size = static_cast<int>(len);
        sock = fd;
        return true;
    }
};

mutex g_mutex{ };
queue<Message> g_msgQue{ };
atomic<bool> g_sentOver(false);         // set by the sender when the file is queued; workers exit on it

uint64_t getUsecTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64_t>(tv.tv_sec) * 1000000ULL + static_cast<uint64_t>(tv.tv_usec);
}

// refresh the rate counter once per second
void tickRate(uint64_t& stamp, uint64_t& bytes)
{
    uint64_t now = getUsecTime();
    if (now - stamp > 1000000ULL) {
        g_state.fBps = static_cast<float>(bytes) / static_cast<float>(now - stamp) * 1000000.f;
        stamp = now;
        bytes = 0;
    }
}

void queuePush(Message&& msg)
{
    lock_guard<mutex> guard(g_mutex);
    g_msgQue.push(std::move(msg));
}

int queuePop(Message& msg)
{
    lock_guard<mutex> guard(g_mutex);
    if (g_msgQue.empty()) {
        return -1;
    }
    msg = std::move(g_msgQue.front());
    g_msgQue.pop();
    return 0;
}

size_t queueSize()
{
    lock_guard<mutex> guard(g_mutex);
    return g_msgQue.size();
}

void wait(unsigned int tms)
{
    struct timespec ts;
    ts.tv_sec = tms / 1000;
    ts.tv_nsec = static_cast<long>(tms % 1000) * 1000000L;   // split into sec + nsec, avoids tv_nsec overflow
    while (nanosleep(&ts, &ts) < 0 && errno == EINTR) {
    }
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

static bool is_multicast_addr(const char* ip)
{
    in_addr_t a = inet_addr(ip);
    if (a == INADDR_NONE) return false;
    uint32_t h = ntohl(a);
    return (h >= 0xE0000000 && h <= 0xEFFFFFFF); // 224.0.0.0 - 239.255.255.255
}

int server(int argc, char* argv[])
{
    const char* file = "./test.dat";
    const char* mgroup = NULL;
    g_state.bserv = true;
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <listen port> [TCP(1)/UDP(0)/MCAST(2)] [pkgsize] [save filename(0 if no save)] [mcast_group]" << endl;
        return -1;
    }
    g_state.port = atoi(argv[1]);
    if (g_state.port <= 0 || g_state.port > 65535) {
        fprintf(stderr, "invalid port: %s\n", argv[1]);
        return -1;
    }
    g_state.how = (argc > 2) ? static_cast<TCPUDP>(atoi(argv[2])) : TCP;
    int pkgsize = 1024;
    if (argc > 3) {
        pkgsize = atoi(argv[3]);
        if (pkgsize > MAX_PKG_SIZE) {
            pkgsize = MAX_PKG_SIZE;
            cout << "message size too big, fixed to 4M." << endl;
        }
        if (pkgsize <= 0) {
            pkgsize = 1024;                     // negative input would become a huge size_t length, clamp it
            cout << "message size invalid, fixed to 1024." << endl;
        }
    }
    if (argc > 4) {
        file = argv[4];
        if (file[0] != '0') {
            g_state.dealFile = true;
        }
    }
    if (argc > 5) {
        mgroup = argv[5];
    }
    int ssock = -1;
    if (g_state.how == TCP) {
        if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) == ~0) {
            perror("socket");
            return -4;
        }
    } else {
        ssock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (ssock < 0) {
            perror("socket");
            return -4;
        }
        // Allow multiple listeners on same machine/port (needed for multicast)
        int reuse = 1;
        if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, sizeof(reuse)) < 0) {
            perror("setsockopt SO_REUSEADDR");
            // non-fatal
        }
    }
    g_state.sock = ssock;
    cout << "server start by " << (g_state.how == TCP ? "TCP" : (g_state.how == MCAST ? "MCAST" : "UDP"))
        << ", pkgsize:" << pkgsize << (g_state.dealFile ? ", write to '" + string(file) + "'" : "")
        << (mgroup ? (", join multicast '" + string(mgroup) + "'") : "") << " ok." << endl;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(static_cast<uint16_t>(g_state.port));
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(ssock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        close(ssock);
        perror("bind");
        return -5;
    }

    // If multicast group specified and IPv4 multicast, join the group
    if (g_state.how == MCAST && mgroup != NULL && is_multicast_addr(mgroup)) {
        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(mgroup);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(ssock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            perror("setsockopt IP_ADD_MEMBERSHIP");
            // proceed anyway
        } else {
            cout << "joined multicast group " << mgroup << endl;
        }
    }

    cout << "socket bind port " << g_state.port << " ok." << endl;
    if (g_state.how == TCP) {
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
    vector<unsigned char> msgbuf(static_cast<size_t>(pkgsize));
    char ip[16];
    fd_set fds;
    FD_ZERO(&fds);
    while (g_state.running) {
        if (g_state.how == TCP) {
            int csock = accept(ssock, (struct sockaddr*)&local, &locsize);
            if (csock < 0) {
                if (errno == EINTR) {
                    continue;
                }
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
            while (g_state.running) {
                FD_ZERO(&fds);                      // rebuild the fd set every round; select rewrites timeout too
                FD_SET(csock, &fds);
                timeout.tv_sec = 0;
                timeout.tv_usec = 3000;
                int ready = select(csock + 1, &fds, NULL, NULL, &timeout);
                if (ready < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    perror("select");
                    break;
                }
                if (ready == 0 || !FD_ISSET(csock, &fds)) {
                    continue;
                }
                ssize_t rcvlen = ::recv(csock, (char*)msgbuf.data(), static_cast<size_t>(pkgsize), 0);
                if (rcvlen > 0) {
                    calclen += static_cast<uint64_t>(rcvlen);
                    total += static_cast<uint64_t>(rcvlen);
                    tickRate(start, calclen);
                    if (g_state.dealFile && g_state.filep != NULL) {
                        size_t i_write_count = fwrite(msgbuf.data(), 1, static_cast<size_t>(rcvlen), g_state.filep);
                        if (i_write_count != static_cast<size_t>(rcvlen)) {
                            fprintf(stderr, "recv data write failed: %s, write(count=%zu,size=%zd).\n", strerror(errno), i_write_count, rcvlen);
                            fclose(g_state.filep);
                            g_state.filep = NULL;
                        } else if (rcvlen < 0x10000) {
                            fsync(fileno(g_state.filep));   // flush small packets; filep is NULL after a write error
                        }
                    }
                } else if (rcvlen == 0) {
                    sync();
                    if (g_state.filep) {
                        fclose(g_state.filep);
                        g_state.filep = NULL;
                    }
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
            FD_ZERO(&fds);
            FD_SET(ssock, &fds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 3000;
            if (select(ssock + 1, &fds, NULL, NULL, &timeout) > 0) {
                if (FD_ISSET(ssock, &fds) > 0) {
                    ssize_t rcvlen = 0;
                    struct sockaddr_in loc;
                    socklen_t loclen = sizeof(loc);
                    if ((rcvlen = ::recvfrom(ssock, (char*)msgbuf.data(), static_cast<size_t>(pkgsize), 0, (struct sockaddr*)&loc, &loclen)) < 0) {
                        continue;
                    }
                    calclen += static_cast<uint64_t>(rcvlen);
                    tickRate(start, calclen);
                    inet_ntop(AF_INET, (void*)&loc.sin_addr, ip, sizeof(ip));
                    printf("recv from %s:%d size=%zd\n", ip, ntohs(loc.sin_port), rcvlen);
                    for (ssize_t i = 0; i < rcvlen; i++) {
                        if ((i % 32 == 0) && (i > 0))
                            printf("\n");
                        printf("%02x ", msgbuf[static_cast<size_t>(i)]);
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
    cout << "Usage: " << prog << " <IP> <server port> [TCP(1, default)/UDP(0)] [package size(64B, default)] [send filename] [thread count]" << endl;
}

int client(int argc, char* argv[])
{
    int ssock = -1;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    int caplen = 1024;
    const char* file = "./test";
    size_t thrds = 1;
    g_state.bserv = false;
    vector<int> vecstat;
    vector<char> msgbuf(1024);
    if (argc <= 2) {
        usage(argv[0]);
        return -1;
    }
    const char* ip = argv[1];
    local.sin_addr.s_addr = inet_addr(ip);
    if (local.sin_addr.s_addr == INADDR_NONE && strcmp(ip, "255.255.255.255") != 0) {
        fprintf(stderr, "invalid ip: %s\n", ip);
        return -1;
    }
    g_state.port = atoi(argv[2]);
    if (g_state.port <= 0 || g_state.port > 65535) {
        fprintf(stderr, "invalid port: %s\n", argv[2]);
        return -1;
    }
    local.sin_port = htons(static_cast<uint16_t>(g_state.port));
    if (argc > 3) {
        g_state.how = static_cast<TCPUDP>(atoi(argv[3]));
    }
    if (argc > 4) {
        caplen = atoi(argv[4]);
        if (caplen > MAX_PKG_SIZE) {
            caplen = MAX_PKG_SIZE;
            cout << "message caplen too big, fixed to 4M." << endl;
        }
        if (caplen <= 0) {
            caplen = static_cast<int>(msgbuf.size());
            cout << "message caplen invalid, fixed to " << caplen << "." << endl;
        }
        msgbuf.resize(static_cast<size_t>(caplen));   // let --package size really drive the per-send packet size
    }
    if (argc > 5) {
        file = argv[5];
        g_state.dealFile = true;
    }
    cout << "client start by " << (g_state.how == TCP ? "TCP" : (g_state.how == MCAST ? "MCAST" : "UDP")) << " send-to " << ip << ":" << g_state.port << " caplen=" << caplen << (g_state.dealFile ? ", file=" + string(file) : "") << " ok." << endl;
    bool is_mcast = is_multicast_addr(ip);
    if (argc > 6) {
        thrds = static_cast<size_t>(atoi(argv[6]));
        if (thrds < 1) {
            thrds = 1;
        }
        if (thrds > MAX_WORK_THREADS) {
            thrds = MAX_WORK_THREADS;
            cout << "thread count too big, fixed to " << MAX_WORK_THREADS << "." << endl;
        }
    }
    if (thrds > 1 && g_state.how == TCP) {      // UDP sends via sendto directly, no worker threads needed
        vecstat.assign(thrds, 0);
        thread* works = new thread[thrds];
        for (size_t i = 0; i < thrds; i++) {
            works[i] = thread([&](size_t index) -> void {
                Message msg;
                uint64_t total = 0;
                while (!g_sentOver || queueSize() > 0) {   // exit only after the sender is done and the queue drains
                    if (queuePop(msg) < 0) {
                        wait(1);
                        continue;
                    }
                    if (msg.sock > 0 && msg.size > 0 && msg.addr != nullptr) {
                        ssize_t bytes = send(msg.sock, msg.addr, static_cast<size_t>(msg.size), 0);
                        if (bytes < 0) {
                            fprintf(stderr, "send failed, socket=%d, size=%d.\n", msg.sock, msg.size);
                        } else {
                            total += static_cast<uint64_t>(bytes);
                        }
                    }
                    msg.release();
                }
                vecstat[index] = 1;             // separate storage per element, avoids vector<bool> bit sharing
                cout << "work thread[" << index << "] total size = " << total << endl;
                },
                i);
            works[i].detach();
            cout << "work thread[" << i << "] start" << endl;
        }
        delete[] works;
    }
    if (g_state.how == TCP) {
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
        if (ssock < 0) {
            perror("socket");
            return -1;
        }
        // If sending to multicast group, set TTL and optionally enable loopback if desired
        if (is_mcast) {
            unsigned char ttl = 32; // Set multicast TTL to 32
            if (setsockopt(ssock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
                perror("setsockopt IP_MULTICAST_TTL");
            }
            // Ensure sender's packets are allowed to loop back - optional, leave default
            // unsigned char loop = 1;
            // setsockopt(ssock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        }
    }
    if (argc <= 5) {
        cout << "type message to send:" << endl;
        while (cin >> msgbuf.data()) {
            size_t len = strnlen(msgbuf.data(), msgbuf.size() - 1) + 1;
            msgbuf[msgbuf.size() - 1] = '\0'; // Ensure null-termination
            if (g_state.how == TCP) {
                ssize_t bytes = send(ssock, msgbuf.data(), len, 0);
                if (bytes < 0) {
                    perror("send");
                    continue;
                }
            } else {
                ::sendto(ssock, msgbuf.data(), len, 0, (struct sockaddr*)&local, sizeof(local));
            }
            cout << "sent [" << msgbuf.data() << "] to " << ip << endl;
        }
    } else {
        g_sentOver = false;
        g_state.fileno = open(file, O_RDONLY, 0666);
        if (g_state.fileno != -1) {
            uint64_t sentSize = 0;
            long total = lseek(g_state.fileno, 0, SEEK_END);
            lseek(g_state.fileno, 0, SEEK_SET);
            uint64_t start = getUsecTime();
            uint64_t current = start;
            uint64_t calcsize = 0;
            ssize_t rdsize = 0;
            size_t chunk = msgbuf.size();          // caplen was already clamped and resized while parsing args
            while ((rdsize = read(g_state.fileno, msgbuf.data(), chunk)) > 0) {
                if (rdsize > static_cast<ssize_t>(chunk)) {
                    // read() must not return more than requested; stop if it does
                    fprintf(stderr, "read beyond buffer: got %zd, expect <= %zu\n", rdsize, chunk);
                    break;
                }
                if (rdsize != static_cast<ssize_t>(chunk)) {
                    fprintf(stdout, "read last size=%zd, expect=%zu: error: %s\n", rdsize, chunk, strerror(errno));
                }
                if (g_state.how == TCP) {
                    if (thrds > 1) {
                        while (queueSize() >= static_cast<size_t>(MAX_QUEUE_SIZE)) {
                            wait(1);                    // back-pressure: wait for workers when the queue is full
                        }
                        Message msg;
                        if (!msg.assign(msgbuf.data(), static_cast<size_t>(rdsize), ssock)) {
                            fprintf(stderr, "malloc failed, drop %zd bytes.\n", rdsize);
                            break;
                        }
                        queuePush(std::move(msg));
                    } else {
                        ssize_t bytes = send(ssock, msgbuf.data(), static_cast<size_t>(rdsize), 0);
                        if (bytes < 0) {
                            perror("send");
                            continue;
                        }
                        sentSize += static_cast<uint64_t>(bytes);
                        calcsize += static_cast<uint64_t>(bytes);
                        if (getUsecTime() - current > 1000000ULL) {
                            g_state.fBps = static_cast<float>(calcsize) / static_cast<float>(getUsecTime() - current) * 1000000.f;
                            g_state.progress = static_cast<float>(sentSize) / static_cast<float>(total);
                            current = getUsecTime();
                            calcsize = 0;
                        }
                    }
                } else {
                    ssize_t sent = ::sendto(ssock, msgbuf.data(), static_cast<size_t>(rdsize), 0,
                        (struct sockaddr*)&local, sizeof(local));
                    if (sent < 0) {
                        perror("sendto");
                    } else {
                        sentSize += static_cast<uint64_t>(sent);
                    }
                }
            }
            g_sentOver = true;                  // everything is queued, tell the workers they may exit
            while (thrds > 1 && g_state.how == TCP) {
                bool status = true;
                for (size_t i = 0; i < vecstat.size(); i++) {
                    status = status && (vecstat[i] != 0);
                }
                if (status) {
                    break;
                }
                wait(1);                        // sleep instead of spinning, keep the CPU free
            }
            fprintf(stdout, "sent %.3fM over, average speed is %.3f MB/s\n",
                static_cast<double>(sentSize) / 1048576.0,
                static_cast<double>(sentSize) / static_cast<double>(getUsecTime() - start) * 1048576.0 / 1000000.0);
            close(g_state.fileno);
        } else {
            perror("open");
            g_sentOver = true;              // nothing to send, do not keep the workers waiting
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
                        fprintf(stdout, "recvd speed %.3f %s\r", static_cast<double>(value), unit.c_str());
                        fflush(stdout);
                        lastValue = value;
                        lastUnit = unit;
                    }
                } else {
                    if (lastValue != value || lastUnit != unit) {
                        fprintf(stdout, "sent %3.3f%% speed %.3f %s\r",
                            static_cast<double>(g_state.progress) * 100.0,
                            static_cast<double>(value), unit.c_str());
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
