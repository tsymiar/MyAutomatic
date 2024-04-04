#include <cstdlib>
#include <iostream>
#include <time.h>
#ifdef _WIN32
#pragma comment(lib, "WS2_32.lib")
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#endif
#define NE_VAL(a) (((~((a) & 0x0f)) | ((a) & 0xf0)) & 0xff)
#define DEFAULT_PORT 8877
#define IPC_KEY 0x520905
#define IPC_FLAG IPC_CREAT|IPC_EXCL //|SHM_R|SHM_W
#define THREAD_NUM 24
#define ACC_REC "accounts"
#define MAX_USERS 99
#define MAX_ZONES 9
#define MAX_MEMBERS_PER_GROUP 11
#if (! defined __APPLE__)
#define MAX_ACTIVE 30
#else
#define MAX_ACTIVE 6 // ??
#endif
//using namespace std;
//'C++11 std' bind func conflicts with in 'socket.h'
#ifdef _WIN32
typedef int type_len;
typedef SOCKET type_socket;
#ifndef pthread_t
typedef unsigned int pthread_t;
#endif
typedef unsigned int type_thread_func;
#ifndef ssize_t
typedef long ssize_t;
#endif
#define flush_all() _flushall()
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS
#define SLEEP(t) Sleep((DWORD)t);
#define _CAS(x,y,z) InterlockedCompareExchange((LONG volatile*)x, (LONG)y, (LONG)z)
#else
typedef int type_socket;
typedef socklen_t type_len;
typedef void* type_thread_func;
#define flush_all() ;
#define closesocket(socket) close(socket)
#define SLEEP(t) wait(t < 0.1f ? 1 : (int)(1.0f*(t)));
pthread_mutexattr_t attr;
#define _CAS __sync_bool_compare_and_swap
#endif

/*-------------------- Message Structure --------------------*/
/* ._______________________________________________________________________________________________________________________________________________________. */
/* |  rsv(1)  |  uiCmdMsg(1)  |  rtn(2)  |  chk(4)  |  usr(24)  | psw/TOKEN/peerIP(24) | peer/port/sign/npsw |     PeerStruct peer_msg     |  status (8)  | */
/* | reserved | cmd msg of ui | error No | checksum | user name | cert or udp peer ip  | host/join/seek(24)  | rsv(2) msg(16) cmd(2) val(4) | if nessasery | */
/* + ———————————————————————————————————————————————————————————————————————————+ */

#define __ "./"
#define _0_ "_0"
#define IMAGE_BLOB "image"
#define GET_IMG_EXE "snap.exe"
#define PRINT_RECV(msg, cmd, chk, usr, flg, idx, txt) do { \
    fprintf(stdout, "----------------------------------------------------------------\\ \
                        \n>>> %s [%0x,%0x]: %s, %d, %u\n", msg, cmd, static_cast<unsigned>(*chk), usr, flg, (idx)); \
    for (int c = 0; c < flg; c++) { \
        if (c > 0 && c % 32 == 0) \
            fprintf(stdout, "\n"); \
        fprintf(stdout, "%02x ", static_cast<unsigned char>(txt[c])); \
    } \
    fprintf(stdout, "\n"); \
} while (0);

#ifdef _WIN32
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
mutex_allow;

int aim2exit = 0;
type_socket listen_socket;
static unsigned int g_threadNo_ = 0;
static int g_filedes[2];
const int FiledSize = 24;

struct Network {
    volatile unsigned char ip[INET_ADDRSTRLEN];
    volatile unsigned int port;
    volatile type_socket socket;
};

struct ONLINE {
    char user[FiledSize];
    Network netwk;
}
#ifdef _WIN32
active[MAX_ACTIVE]
#endif // _WIN32
;

struct user_clazz {
    char usr[FiledSize];
    char psw[FiledSize];
    Network netwk;
    unsigned char hgrp[FiledSize];
    char intro[FiledSize];
}users[MAX_USERS];

struct member {
    user_clazz user[MAX_MEMBERS_PER_GROUP];

    member()
    {
        memset(this, 0, sizeof(*this));
    }
};

typedef struct UsrMsgStu {
    unsigned char rsv;
    unsigned char uiCmdMsg;
    unsigned char rtn[2];
    unsigned char chk[4];
    char usr[FiledSize];
    union {
        char psw[FiledSize];
        char TOKEN[FiledSize];
        char peerIp[FiledSize];
    };
    union {
        char peer[FiledSize];
        // peer port
        char port[FiledSize];
        char sign[FiledSize];
        char npsw[FiledSize];
        // zone to user
        unsigned char host[FiledSize];
        unsigned char join[FiledSize];
        unsigned char seek[FiledSize];
    };
    class PeerStruct {
    private:
        unsigned char rsv[2];
    public:
        PeerStruct() :msg{ 0 }, rsv{ 0 } {};
        ~PeerStruct() {};
        char msg[16];
        unsigned char cmd[2]{ 0 };
        unsigned char val[4]{ 0 };
    } peer_msg;
    char status[8];
} USER;

struct zone_st {
    char name[FiledSize];
    char cert[FiledSize];
    char members[MAX_MEMBERS_PER_GROUP][FiledSize];
    unsigned int usrCnt;
    char chief[FiledSize];
    char brief[FiledSize];
};

struct zone_file {
    zone_st zone;
    zone_file()
    {
        memset(this, 0, sizeof(*this));
    }
    unsigned int getSize() const { return sizeof(*this) + (int)(zone.usrCnt * sizeof(member)); }
}zones[MAX_ZONES];

void pipesig_handler(int s)
{
#ifdef _UNISTD_H
    write(STDERR_FILENO, "Signal: caught a SIGPIPE!\n", 27); // 27: write buffer size.
    // exit(0);
#endif
}

enum {
    CHECK = 0x01,
    USERNAME,
    LOGOUT,
    PASSWD,
    ONLINE,
    P2P,
    NDT,
    EXEC,
    IMAGE,
    ZONES,
    MEMBERS,
    HOST_ZONE,
    JOIN_ZONE,
    EXIT_ZONE,
    ALL_ZONE = 0x0f,
};

int inst_mssg(int argc, char* argv[]);
template<typename T> int set_n_get_mem(T* shmem, int ndx = 0, int rw = 0);
void func_waitpid(int signo);
int queryNetworkParams(const char* username, Network& network, const int max = MAX_ACTIVE);
Network queryNetworkParams(int uid);
type_thread_func commands(void* arg);
void wait(unsigned int tms);
int user_auth(char usr[FiledSize], char psw[FiledSize]);
int new_user(char usr[FiledSize], char psw[FiledSize]);
int user_is_line(char user[FiledSize]);
int set_user_line(char user[FiledSize], Network& netwk);
int set_user_peer(const char user[FiledSize], const char ip[INET_ADDRSTRLEN], const int port, type_socket sock);
int set_user_quit(char user[FiledSize]);
int get_user_seq(char user[FiledSize]);
int save_accnt();
int load_accnt();
int find_zone(unsigned char basis[FiledSize]);
int host_zone(USER& user);
int join_zone(int at, char usr[FiledSize], char zone[FiledSize], char* cert = NULL);
int exit_zone(int at, char usr[FiledSize]);
int free_zone(int at, char host[FiledSize]);

int main(int argc, char* argv[])
{
    fprintf(stdout, "------- IM chat server v0.%d for %dbit OS. -------\n", 1, static_cast<int>(sizeof(void*)) * 8);
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "-m") {
            struct ONLINE active[MAX_ACTIVE];
            memset(active, 0, sizeof(active));
            for (int c = 0; c < MAX_ACTIVE; c++) {
                set_n_get_mem(&active[c], c);
            }
            fprintf(stdout, "\tNo.\tuser\tIP\t\tPort\tsocket\n");
            for (int c = 0; c < MAX_ACTIVE; c++) {
                if (active[c].user[0] != '\0') {
                    fprintf(stdout, "\t%d\t%s\t%s\t%u\t%d\n", c + 1,
                        active[c].user, active[c].netwk.ip, active[c].netwk.port, active[c].netwk.socket);
                }
            }
            exit(0);
        }
        if (arg1 == "-x") {
            struct ONLINE active[MAX_ACTIVE];
            memset(active, 0, sizeof(active));
            for (int c = 0; c < MAX_ACTIVE; c++) {
                set_n_get_mem(&active[c], c, -1);
            }
            exit(0);
        }
    }
#ifdef _WIN32
    inst_mssg(1, { nullptr });
#else
    if (pipe(g_filedes) < 0) {
        fprintf(stderr, "Make pipe channel fail: %s\n", strerror(errno));
    }
    if (fork() == 0) {
        signal(SIGPIPE, pipesig_handler);
        inst_mssg(argc, argv);
    }
#endif
    return 0;
}

type_thread_func monitor(void* arg)
{
    static USER user = {};
    constexpr int BUFF_SIZE = 256;
    int c, cnt = 0;
    int qq = 0, loggedIn = 0;
    int val_rtn = -1;
    char sd_bufs[BUFF_SIZE], rcv_txt[BUFF_SIZE];
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];
    memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
#endif
    type_socket rcv_sock = 0;
    type_socket* sock = reinterpret_cast<type_socket*>(arg);
    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peerAddr;
    socklen_t peerLen = static_cast<socklen_t>(sizeof(peerAddr));
    if (arg && -1 != long(sock) && int(*sock) != 0) {
        rcv_sock = *sock;
    } else {
#if (!defined THREAD_PER_CONN) && ((!defined _WIN32 ) || (defined SOCK_CONN_TEST))
        std::cerr << "ERROR: socket contra-valid([" << sock << "]" << (sock != nullptr ? *sock : -1) << ") " << strerror(errno) << std::endl;
        exit(-1);
#else
        struct sockaddr_in sin;
        type_len len = static_cast<type_len>(sizeof(sin));
        rcv_sock = accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
#endif
    }
    getpeername(rcv_sock, reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);
    const char* IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
    const int PORT = ntohs(peerAddr.sin_port);
    time_t t;
    struct tm* lt;
    time(&t);
    lt = localtime(&t);
    g_threadNo_++;
    fprintf(stdout, "Accepted peer(%u) address [%s:%d] (@ %d/%02d/%02d-%02d:%02d:%02d)\n", g_threadNo_, IP, PORT, lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    bool set = true;
    setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&set), sizeof(bool));
    if (rcv_sock
#ifdef _WIN32
        == INVALID_SOCKET) {
        std::cerr << "Accept failure ERROR " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        < 0) {
#endif
        std::cerr << "Call accept() error(" << errno << "): " << strerror(errno) << std::endl;
        return type_thread_func(-1);
    };
    fprintf(stdout, "Socket monitor: %d; waiting for massage...\n", rcv_sock);
    do {
        memset(sd_bufs, 0, BUFF_SIZE);
        memset(rcv_txt, 0, BUFF_SIZE);
        memset(&user, 0, sizeof(user));
#ifdef _WIN32
        int
#else
        socklen_t
#endif
            lol = sizeof(int);
        int val;
        if (getsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&val), &lol) == 0) {
            if (cnt > 0)
                fprintf(stderr, "### Connect--%d status changed, socket: %s.\n", cnt, strerror(errno));
            if (errno != 0) {
                set_user_quit(user.usr);
#if defined _WIN32
                ;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                goto comm_err1;
#endif
            }
            cnt++;
        }
        int flg = (int)recv(rcv_sock, rcv_txt, BUFF_SIZE, 0);
        // rcv_txt: inc--8 bit crc_head, 24 bit username, 24 bit password.
        if (qq != flg && flg != 0) {
            if (flg <= -1 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                set_user_quit(user.usr);
                fprintf(stderr, "----------------------------------------------------------------\
                    \n### Client socket [%u] closed by itself just now.\n", (unsigned)rcv_sock);
                closesocket(rcv_sock);
#ifdef _WIN32
                break;
#else
                exit(0);
#endif
            } else {
#ifdef _DEBUG
                memcpy(&user, rcv_txt, sizeof(user));
                PRINT_RECV("1-RCV", user.uiCmdMsg, user.chk, user.usr, flg, THREAD_NUM - g_threadNo_, rcv_txt);
#endif
#ifndef _WIN32
                if (g_filedes[0] > 0)
                    close(g_filedes[0]);
                if (g_filedes[1] <= 0 || write(g_filedes[1], (void*)&rcv_sock, sizeof(rcv_sock)) < 0) {
                    fprintf(stderr, "Can't write socket to filedes[1][%d]: %s\n", g_filedes[1], strerror(errno));
                }
#endif
            }
            ssize_t sn_stat = -1;
            const int offset = 8;
            char userName[FiledSize];
            memset(userName, 0, FiledSize);
            memcpy(sd_bufs, &user, 2); // 2: head bytes
            if ((user.rsv == 0) && (user.uiCmdMsg == 0)) {
                // user.usr: 8; user.psw: 32.
                val_rtn = new_user(user.usr, user.psw);
                snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn + 1));
                if (val_rtn == -1) {
                    snprintf(sd_bufs + offset, 37, "New user: %s", user.usr);
                    join_zone(0, user.usr, const_cast<char*>("all"));
                    sn_stat = send(rcv_sock, sd_bufs, 48, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + offset);
                } else if (val_rtn == -2) {
                    snprintf((sd_bufs + offset), 16, "%s", "Too many users.");
                    sn_stat = send(rcv_sock, sd_bufs, 28, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + offset);
                } else if (val_rtn >= 0) {
                    snprintf((sd_bufs + offset), 21, "%s", "User already exists.");
                    sn_stat = send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + offset);
                } else if (val_rtn == -3) {
                    snprintf((sd_bufs + offset), 22, "%s", "Same user name exist.");
                    sn_stat = send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + offset);
                } else if (val_rtn == -4) {
                    snprintf((sd_bufs + offset), 17, "%s", "User name error.");
                    sn_stat = send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + offset);
                };
                if (sn_stat < 0) {
                    fprintf(stderr, "### Socket status: %s\n", strerror(errno));
                }
                continue;
            } else if ((user.rsv == 0) && (user.uiCmdMsg == 0x1)) {
                val_rtn = user_auth(user.usr, user.psw);
                snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                if (val_rtn == (loggedIn = 1)) {
                    snprintf(sd_bufs + offset, 54, "[%s] logging on successfully.", user.usr);
                    set_user_peer(user.usr, IP, PORT, rcv_sock);
                    Network sock;
                    static_cast<void>(memcpy((void*)(sock.ip), IP, INET_ADDRSTRLEN)),
                        static_cast<void>(sock.port = PORT),
                        sock.socket = rcv_sock;
                    set_user_line(user.usr, sock);
                    memcpy(userName, user.usr, FiledSize);
                    send(rcv_sock, sd_bufs, 64, 0);
                } else if (val_rtn == 0) {
                    if (memcmp(user.usr, userName, FiledSize) == 0) {
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(-3));
                        snprintf(sd_bufs + offset, 49, "Another [%s] is on line.", user.usr);
                        if (send(rcv_sock, sd_bufs, 64, 0) < 0) {
                            fprintf(stderr, "### User status: %s\n", strerror(errno));
#if defined _WIN32
                            return NULL;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                            goto comm_err1;
#endif
                        } else
                            fprintf(stdout, ">>> %s\n", sd_bufs + offset);
                        continue;
                    } else {
                        snprintf((sd_bufs + 2), 8, "%02x", 0xe8);
                        snprintf((sd_bufs + offset), 52, "Logging status invalid, we will close this connect.");
                        set_user_quit(user.usr);
                        sn_stat = send(rcv_sock, sd_bufs, 64, 0);
                        closesocket(rcv_sock);
                        fprintf(stdout, ">>>%s %s\n", sn_stat < 0 ? strerror(errno) : "", sd_bufs + offset);
                        loggedIn = 0;
                        continue;
                    }
                } else if (val_rtn < 0) {
                    snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                    snprintf((sd_bufs + offset), 38, "%s", "Error: check username/password again.");
                    sn_stat = send(rcv_sock, sd_bufs, 48, 0);
                    fprintf(stdout, ">>>%s %s\n", sn_stat < 0 ? strerror(errno) : "", sd_bufs + offset);
                    continue;
                } else {
                    memset((sd_bufs + offset), 0, 64);
                    fprintf(stdout, ">>> Unknown Error!\n");
                    continue;
                };
            } else if ((user.rsv == 0) && (user.uiCmdMsg == 0x3)) {
                snprintf(sd_bufs + 2, 8, "%x", NE_VAL(-1));
                snprintf((sd_bufs + offset), 36, "%s", "Warning: user hasn't logged on yet.");
                sn_stat = send(rcv_sock, sd_bufs, 48, 0);
                fprintf(stdout, ">>>%s %s\n", sn_stat < 0 ? strerror(errno) : "", sd_bufs + offset);
                continue;
            } else {
                snprintf((sd_bufs + offset), 50, "%s", "Warning: please Register(0) or Login(1) at first.");
                snprintf(sd_bufs + 2, 8, "%x", NE_VAL(-2));
                sn_stat = send(rcv_sock, sd_bufs, 58, 0);
                fprintf(stdout, ">>>%s %s\n", sn_stat < 0 ? strerror(errno) : "", sd_bufs + offset);
                continue;
            }
#ifdef _DEBUG
            fprintf(stdout, ">>> 1-MSG [%0x,%0x]: ", sd_bufs[0], sd_bufs[1]);
            for (c = 2; c < 64; c++)
                fprintf(stdout, "%c", static_cast<unsigned char>(sd_bufs[c]));
            fprintf(stdout, "\n");
#endif
            // set current qq as last flg;
            qq = flg;
            while (loggedIn) {
                time_t t1 = t;
                time(&t);
                // heart beat
                if ((t - t1 > 30)
                    && (send(rcv_sock, "\0\0", 2, 0) < 0)) {
#if defined _WIN32
                    return NULL;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                    goto comm_err1;
#endif
                }
                memset(&user, 0, sizeof(user));
                flg = (int)recv(rcv_sock, rcv_txt, BUFF_SIZE, 0);
                if (flg < 0 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                    set_user_quit(userName);
                    fprintf(stderr, "### Lost connection with *[%s]: %s(%d)\n", userName, strerror(errno), flg);
#if defined _WIN32 || !defined THREAD_PER_CONN || !defined SOCK_CONN_TEST
                    return NULL;
#else
                    loggedIn = 0;
                    goto comm_err1;
#endif
                } else {
                    if (flg < 8 && flg != 2) {
                        set_user_quit(userName);
                        if (flg == 0) {
                            fprintf(stderr, "### Peer socket was missing...\n");
#if defined _WIN32
                            return NULL;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                            goto comm_err1;
#endif
                        } else {
                            fprintf(stderr, "### Request parameter invalid.\n");
                            goto comm_err0;
                        }
                    }
                    memcpy(&user, rcv_txt, flg);
                }
                if (flg > 0) {
#ifdef _DEBUG
                    PRINT_RECV("2-RCV", user.uiCmdMsg, user.chk, user.usr, flg, THREAD_NUM - g_threadNo_, rcv_txt);
#endif
                }
                unsigned long total = BUFF_SIZE;
                memset(sd_bufs, 0, total);
                if (user.rsv == 0) {
                    switch (sd_bufs[1] = user.uiCmdMsg) {
                    case CHECK:
                        snprintf((sd_bufs + offset), 26, "%s", "User has already on-line.");
                        total = 48;
                        break;
                    case USERNAME:
                        char mark[sizeof(user)];
                        snprintf(mark, 18 + 24 * 2, "User: %s(--%s--);", user.usr, user.sign);
                        mark[18 + 24 * 2 - 1] = '\0';
                        snprintf((sd_bufs + offset), sizeof(mark), "%s", mark);
                        total = offset + sizeof(mark);
                        break;
                    case LOGOUT:
                    {
                        flg = loggedIn = 0;
                        set_user_quit(userName);
                        snprintf(sd_bufs + offset, 17 + 24, "[%s] has logout.", userName);
                        sn_stat = send(rcv_sock, sd_bufs, 48, 0);
                        fprintf(stderr, "### [%0x, %x]:%s %s\n", sd_bufs[1],
                            static_cast<unsigned>(*user.chk), sn_stat < 0 ? strerror(errno) : "", sd_bufs + offset);
                        continue;
                    }
                    case PASSWD:
                    {
                        val_rtn = get_user_seq(user.usr);
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        if (1 == user_auth(user.usr, user.psw) && user.npsw[0] != '\0')
                            memcpy(users[val_rtn].psw, user.npsw, 24);
                        else
                            snprintf((sd_bufs + offset), 42, "%s", "Change password failure: user auth error.");
                        total = 56;
                    } break;
                    case ONLINE:
                    {
                        snprintf((sd_bufs + offset), 22, "%s", "Users on-line list:\n");
                        int n = 0;
                        for (c = 0; c < MAX_ACTIVE; c++) {
                            set_n_get_mem(&active[c], c);
                            if (active[c].user[0] != '\0') {
                                snprintf(sd_bufs + 2, 8, "%x", c);
                                memcpy((sd_bufs + offset * (c + 4)), active[c].user, 24);
                                void* const psr = (sd_bufs + offset * (c + 4) + FiledSize);
                                if ((*reinterpret_cast<char*>(psr)) == '\0') {
                                    memset(psr, '\t', 1);
                                }
                                n++;
                            } else if (active[c].user[0] == '\0')
                                break;
                        };
                        *(sd_bufs + 2) = '0';
                        snprintf(sd_bufs + 4, 4, "%x", n);
                        total = static_cast<unsigned long>(offset * (c + 4 + 4));
                    } break;
                    case P2P:
                    {
                        unsigned int uiIP = 0;
                        memcpy((sd_bufs + offset), user.usr, 24);
                        val_rtn = get_user_seq(user.peer);
                        if (val_rtn >= 0) {
                            if (user_is_line(user.peer) >= 0) {
                                fprintf(stdout, "``` '%s' wants to P2P with '%s'\n", user.usr, user.peer);
                                Network p2pnet = queryNetworkParams(val_rtn);
#if !defined _WIN32
                                queryNetworkParams(user.peer, p2pnet);
#endif
                                const char* s = reinterpret_cast<char*>((unsigned char**)&p2pnet.ip);
                                unsigned char t = 0;
                                while (1) {
                                    if (*s != '\0' && *s != '.') {
                                        t = (unsigned char)(t * 10 + *s - '0');
                                    } else {
                                        uiIP = (uiIP << 8) + t;
                                        if (*s == '\0')
                                            break;
                                        t = 0;
                                    }
                                    s++;
                                };
                                snprintf((sd_bufs + 32), 8, "%u", uiIP);
                                snprintf((sd_bufs + 56), 8, "%u", p2pnet.port);
                                if (memcmp(user.chk, "P2P", 4) == 0) { // recieved a hole digging message
                                    fprintf(stdout, "'%s' is communicating with '%s' via P2P.\n", user.usr, user.peer);
                                    snprintf(sd_bufs + 2, 8, "%x", NE_VAL(0));
                                    memcpy((sd_bufs + 4), "P2P", 4);
                                    snprintf((sd_bufs + 64), 43, "%s request a hole.", user.usr);
                                    if (-1 != send(p2pnet.socket, sd_bufs, 108, 0)) {
                                        fprintf(stdout, "Have sent message to %s.\n", user.peer);
                                    } else {
                                        memset(sd_bufs + offset, 0, 248);
                                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(-1));
                                        snprintf((sd_bufs + 32), 27 + 24, "failed send command to %s.", user.peer);
                                        sn_stat = send(rcv_sock, sd_bufs, total, 0);
                                        fprintf(stdout, "Got failure trans message to %s:%u (%zd,%s).\n", p2pnet.ip, p2pnet.port, sn_stat, strerror(errno));
                                    }
                                    break;
                                }
                                // p2p_req2usr
                            } else {
                                val_rtn = -2;
                                snprintf((sd_bufs + 32), 23, "%s", "P2P peer user offline!");
                            }
                        } else {
                            val_rtn = -3;
                            snprintf((sd_bufs + 32), 36 + 24, "Get user network: No such user(%s)!", user.peer);
                        }
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        unsigned char* val = reinterpret_cast<unsigned char*>(&uiIP);
                        fprintf(stdout, ">>> get client peer [%u.%u.%u.%u:%s]\n", val[3], val[2], val[1], val[0], sd_bufs + 56);
                        total = 96;
                    } break;
                    case NDT:
                    {
                        memcpy((sd_bufs + offset), user.usr, 24);
                        if (memcmp(user.chk, "NDT", 4) == 0) {
                            fprintf(stdout, "'%s' is communicating with '%s' via NDT.\n", user.usr, user.peer);
                        } else {
                            snprintf((sd_bufs + 32), 50, "%s", "Check message detail of Network Data Translation.");
                            total = 88;
                            break;
                        }
                        val_rtn = get_user_seq(user.peer);
                        if (val_rtn >= 0) {
                            if (user_is_line(user.peer) >= 0) {
                                Network ndt_net = queryNetworkParams(val_rtn);
#if !defined _WIN32
                                queryNetworkParams(user.peer, ndt_net);
#endif
                                if (ndt_net.ip[0] == '\0' || ndt_net.port == 0) {
                                    snprintf((sd_bufs + 32), 35, "%s", "Peer user ip or port value error.");
                                    total = 72;
                                } else {
                                    char random = (char)(rand() % 255 + '\1');
                                    char ndt_msg[32];
                                    memset(ndt_msg, 0, 32);
                                    memcpy(ndt_msg, &user, 4);
                                    snprintf(ndt_msg + 4, 8, "%c", random);
                                    memcpy(reinterpret_cast<char*>(&user.status), "200", 4);
                                    memcpy(ndt_msg + offset, &user.peer_msg.msg, FiledSize);
                                    fprintf(stdout, "Command(%d) NDT is: %08x.\n", *(uint16_t*)user.peer_msg.cmd, *(uint32_t*)user.peer_msg.val);
                                    if (-1 != send(ndt_net.socket, ndt_msg, 32, 0)) {
                                        if (ndt_net.socket == rcv_sock) {
                                            snprintf((sd_bufs + 32), 29, "%s", "Socket descriptor conflicts!");
                                            total = 64;
                                        } else {
                                            snprintf(sd_bufs + 32, 49, "[%c] NDT success to %s.", random, user.peer);
                                            total = 80;
                                        }
                                    } else {
                                        struct stat sock_stat;
                                        if (EBADF == fstat(ndt_net.socket, &sock_stat)) {
                                            fprintf(stdout, "Error: socket descriptor - %d.\n", ndt_net.socket);
                                        } else {
                                            fprintf(stdout, "Error: %s(%d).\n", strerror(errno), ndt_net.socket);
                                        }
                                        snprintf((sd_bufs + 32), 38, "%s", "Got failure while sending a message.");
                                        total = 80;
                                    }
                                }
                            } else {
                                snprintf((sd_bufs + 32), 23, "NDT Peer User Offline!");
                                memset(sd_bufs + 2, NE_VAL(-2), 1);
                                total = 72;
                            }
                        } else {
                            snprintf((sd_bufs + 32), 36 + 24, "Get user network: No such user(%s)!", user.peer);
                            snprintf((sd_bufs + 2), 8, "%x", NE_VAL(-3));
                            total = 96;
                        }
                    } break;
                    case EXEC:
                    {
#if !defined _WIN32
                        const char* exe_msg = NULL;
#if (defined __GNUC__ && __APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
                        if (vfork() == 0) { // posix_spawn /* child process first */
#if (defined __GNUC__ && __APPLE__)
#pragma GCC diagnostic pop
#endif
#ifdef RASPI
#undef GET_IMG_EXE
#define GET_IMG_EXE "raspistill"
#undef __
#undef _0_
#define __ ""
#define _0_ ""
                            char* const agv[] = { (char*)GET_IMG_EXE, (char*)" - o", (char*)IMAGE_BLOB, (char*)(0) };
#else
                            char* const agv[] = { const_cast<char*>(GET_IMG_EXE), reinterpret_cast<char*>(0) };
#endif
                            val_rtn = execvp(__ GET_IMG_EXE, agv);
                            exe_msg = strerror(errno);
                            snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                            memcpy((sd_bufs + offset), exe_msg, 72);
                            total = offset + 72 + 1;
                            fprintf(stdout, "Exec [ %s ] with execvp fail, %s.\n", __ GET_IMG_EXE, exe_msg);
                        } else {
                            wait(&val_rtn);
                            if (errno != ECHILD) {
                                exe_msg = strerror(errno);
                            } else {
                                if (access(IMAGE_BLOB _0_, 0) == 0) {
                                    exe_msg = "success";
                                } else {
                                    exe_msg = "Not save image file";
                                }
                            }
                            snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                            memcpy((sd_bufs + offset), exe_msg, strnlen(exe_msg, 256));
                            total = offset + (int)strnlen(exe_msg, 256) + 1;
                            fprintf(stdout, "Make image via '%s': %s.\n", __ GET_IMG_EXE, exe_msg);
                        }
#else
                        char* exe_msg = "OS don't support v4l.";
                        total = 32;
                        memcpy((sd_bufs + offset), exe_msg, strnlen(exe_msg, 256));
                        fprintf(stdout, "%s\n", exe_msg);
#endif
                    } break;
                    case IMAGE:
                    {
                        FILE* file = fopen(IMAGE_BLOB _0_, "rb");
                        if (file == NULL) {
                            snprintf((sd_bufs + offset), 48, "Fail read image file \"%s\".", IMAGE_BLOB);
                            fprintf(stdout, "%s\n", sd_bufs + offset);
                            total = offset + 5 /*len_of(IMAGE_BLOB)*/ + 27;
                            break;
                        }
                        fseek(file, 0, SEEK_END);
                        long lSize = ftell(file);
                        rewind(file);
                        size_t block = (size_t)(lSize / sizeof(unsigned char));
                        if (block > 2097152)
                            block = 4096;
                        memset(sd_bufs + offset, (int)lSize, 6);
                        unsigned char* pos = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * block));
                        if (pos == NULL) {
                            snprintf((sd_bufs + offset), 36, "Fail malloc for \"%s\".", IMAGE_BLOB);
                            fprintf(stdout, "%s\n", sd_bufs + offset);
                            total = offset + 6 + FiledSize;
                            break;
                        }
                        int slice = 0;
                        size_t len = 0;
                        constexpr int CHIP = 224;
                        while (bool rcsz = ((len = fread(pos, sizeof(unsigned char), block, file)) != 0 && !feof(file))
                            || (block > len && len > 0)) {
                            fprintf(stdout, "        "
                                "File \"%s\": total = %ld, slice = %d, read = %zu.\r", IMAGE_BLOB, lSize, slice, len);
                            snprintf((sd_bufs + 14), 8, "%04d", slice);
                            memset(sd_bufs + 1, user.uiCmdMsg, 1);
                            volatile int offset = 0;
                            for (size_t i = 0; i <= len; ++i, ++offset) {
                                if (((i > 0) && (i % CHIP == 0)) || (i == len)) {
                                    snprintf((sd_bufs + 22), 16, "%04zu", i);
                                    send(rcv_sock, sd_bufs, BUFF_SIZE, 0);
                                    memset(sd_bufs + 32, 0, CHIP);
                                    SLEEP(1.0 / 10000);
                                    offset = 0;
                                }
                                memset(sd_bufs + 32 + offset, pos[i], 1);
                                fprintf(stdout, /*"\r%*c*/"\r%.02f%% ", /*7, ' ',*/ i * 100.f / len);
                                fflush(stdout);
                            }
                            slice++;
                        }
                        fprintf(stdout, "\r\n");
                        fclose(file);
                        free(pos);
                    }
                    break;
                    case ZONES:
                    {
                        snprintf((sd_bufs + offset), 21, "%s", "Active group list: \n");
                        int n = 0;
                        for (c = 0; c < MAX_ZONES; c++) {
                            if (zones[c].zone.name[0] != '\0') {
                                snprintf(sd_bufs + 2, 8, "%x", c);
                                memcpy((sd_bufs + offset * (c + 4)), zones[c].zone.name, 24);
                                n++;
                            } else {
                                break;
                            }
                        };
                        *(sd_bufs + 2) = '0';
                        snprintf(sd_bufs + 4, 4, "%x", n);
                        total = static_cast<unsigned long>(offset * (c + 4 + 3));
                    } break;
                    case MEMBERS:
                    {
                        val_rtn = find_zone(user.seek);
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        if (val_rtn == -2) {
                            snprintf((sd_bufs + offset), 44, "No such [%s] group!", user.seek);
                            total = 56;
                        } else {
                            memset(sd_bufs + 2, 0, 2);
                            snprintf((sd_bufs + offset), 25, "%s", "Member(s) of the group:\n");
                            for (c = 0; c < MAX_MEMBERS_PER_GROUP; c++) {
                                if (zones[val_rtn].zone.members[c][0] >> 0) {
                                    memcpy((sd_bufs + offset * (c + 4)), zones[val_rtn].zone.members[c], 24);
                                };
                            };
                            if (val_rtn == -1 || val_rtn >= MAX_ZONES || c >= MAX_MEMBERS_PER_GROUP ||
                                zones[val_rtn].zone.members[c][0] == '\0')
                                break;
                            total = static_cast<unsigned long>(offset * (c + 4 + 4));
                        };
                    } break;
                    case HOST_ZONE:
                    {
                        val_rtn = host_zone(user);
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        if (val_rtn == -2) {
                            snprintf((sd_bufs + offset), 21, "%s", "Host group rejected.");
                            total = 32;
                        } else if (val_rtn < -2 && val_rtn > -MAX_ZONES - 3) {
                            int stat = join_zone(((val_rtn + 3 + MAX_ZONES)), user.usr, reinterpret_cast<char*>(user.join));
                            if (stat >= 0 && user.host[0] != '\0' && user.usr[0] != '\0') {
                                snprintf((sd_bufs + offset), 32 + 24 * 2, "'%s' exist, %s joins the group.", user.host, user.usr);
                                total = 88;
                            } else if (stat == -3) {
                                if (user.join[0] == '\0') {
                                    snprintf((sd_bufs + offset), 29, "Group Name should not empty!");
                                    total = 38;
                                } else {
                                    snprintf((sd_bufs + offset), 31, "User already joined the group!");
                                    total = 36;
                                }
                            } else {
                                snprintf((sd_bufs + offset), 25, "Group parameter invalid!");
                                total = 34;
                            }
                        } else {
                            snprintf((sd_bufs + offset), 56, "Created the group '%s' as host.", user.host);
                            total = 66;
                        }
                    } break;
                    case JOIN_ZONE:
                    {
                        val_rtn = join_zone(find_zone(user.join), user.usr, reinterpret_cast<char*>(user.join));
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        if (val_rtn < 0) {
                            total = 48;
                            switch (val_rtn) {
                            case -1:
                                snprintf((sd_bufs + offset), 51, "Can't find such group: %s.", user.join);
                                total = 58;
                                break;
                            case -2:
                                snprintf((sd_bufs + offset), 45, "Join Limits: group was full for new members.");
                                break;
                            case -3:
                                snprintf((sd_bufs + offset), 36, "You have already joined this group.");
                                break;
                            case -4:
                                snprintf((sd_bufs + offset), 46, "Sth. wrong while passing token to this group.");
                                break;
                            default:
                                snprintf((sd_bufs + offset), 15, "Unknown error.");
                                break;
                            }
                        } else {
                            total = 64;
                            snprintf((sd_bufs + offset), 28 + 24, "Joining group (%s) success.", user.join);
                        }
                    } break;
                    case EXIT_ZONE:
                    {
                        val_rtn = exit_zone(find_zone(user.seek), user.usr);
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        total = 42;
                        if (val_rtn >= 0)
                            snprintf((sd_bufs + offset), 25, "%s", "Leave the group success.");
                        else
                            snprintf((sd_bufs + offset), 32, "%s", "You aren't yet join this group.");
                    } break;
                    case ALL_ZONE:
                    {
                        val_rtn = free_zone(find_zone(user.seek), user.usr);
                        snprintf(sd_bufs + 2, 8, "%x", NE_VAL(val_rtn));
                        for (int i = 0; i < MAX_ACTIVE; i++)
                            set_n_get_mem(&active[i], i);
                        if (val_rtn < 0) {
                            if (-1 != user_is_line(user.usr)) {
                                snprintf(sd_bufs + 3, 9, "%x", -1);
                                memcpy((sd_bufs + offset), user.usr, 24);
                                memcpy((sd_bufs + 32), user.sign, 24);
                                send(rcv_sock, sd_bufs, 48, 0);
                            };
                        } else {
                            for (c = 0; c < MAX_MEMBERS_PER_GROUP; c++) {
                                if (zones[val_rtn].zone.members[c][0] != '\0') {
                                    snprintf(sd_bufs + 3, 9, "%x", -3);
                                    int uil = user_is_line(zones[val_rtn].zone.members[c]);
                                    if (uil != -1) {
                                        memcpy((sd_bufs + offset), user.usr, 24);
                                        memcpy((sd_bufs + 32), user.sign, 24);
                                        send(active[uil].netwk.socket, sd_bufs, 48, 0);
                                    }//if uil
                                }//if zones[val_rtn]
                            }//for MAX_MEMBERS_PER_GROUP
                        }//else val_rtn < 0
                    } break;
                    default: break;
                    }
                    if ((memcmp(user.chk, "P2P", 4) != 0) &&
                        (send(rcv_sock, sd_bufs, total, 0) < 0)) {
                        perror("Socket lost");
#if defined _WIN32
                        return NULL;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                        goto comm_err1;
#endif
                        break;
                    }
#ifdef _DEBUG
                    fprintf(stdout, ">>> 2-MSG [%0x,%0x]: ", sd_bufs[0], sd_bufs[1]);
                    int j = 0;
                    for (c = 2; c < static_cast<int>(sizeof(user)); c++) {
                        if ((c - 32 > 0) && ((c - 32) % 8 == 0)
                            && (sd_bufs[c] == '0' || sd_bufs[c] == '\0' || sd_bufs[c] == '\x20'))
                            fprintf(stdout, " ");
                        if ((c == 0 || c == '0') && j < 4)
                            j++;
                        else
                            j = 0;
                        fprintf(stdout, "%c", static_cast<unsigned char>(sd_bufs[c]));
                    }
                    fprintf(stdout, "\n");
#endif
                }
                SLEEP(99);
            }
        } else {
            closesocket(rcv_sock);
        }
        qq = flg;
        SLEEP(99);
    } while (!loggedIn);

comm_err0:
    {
        for (int i = 0; i < MAX_ACTIVE; i++)
            set_n_get_mem(&active[i], i);
        memset(active[user_is_line(user.usr)].user, 0, FiledSize);
    }
#if !defined _WIN32
    comm_err1 : {
        g_threadNo_--;
        closesocket(rcv_sock);
        exit(errno);
    }
#endif
    return 0;
};

int inst_mssg(int argc, char* argv[])
{
    int srvPort = DEFAULT_PORT;
    if (argc == 2 && atoi(argv[1]) != 0) {
        srvPort = atoi(argv[1]);
    }
    if (!load_accnt()) {
        fprintf(stdout, "account loads finish from [%s].\n", ACC_REC);
    } else {
        memcpy(users[0].usr, "iv9527", 8);
        memcpy(users[0].psw, "tesT123$", 10);
        memcpy(users[1].usr, "AAAAA", 7);
        memcpy(users[1].psw, "AAAAA", 7);
        memcpy(zones[0].zone.name, "all", 5);
        memcpy(zones[0].zone.brief, "all", 5);
        memcpy(zones[0].zone.members[0], "iv9527", 8);
    }
#ifdef _WIN32
    InitializeCriticalSection(&mutex_allow);
#else
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(mutex_allow), &attr);
#endif
    aim2exit = 0;
    pthread_t threadid;
#ifdef _WIN32
    SetConsoleTitle((
#ifdef _UNICODE
        LPCWSTR
#else
        LPCSTR
#endif
        )"chat server for network design");
    _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)commands, nullptr, 0, &threadid);
    WSADATA wsaData;
    if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR) {
        std::cerr << "WSAStartup failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
        return -1;
    }
#endif
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons((uint16_t)srvPort);
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket
#ifdef _WIN32
        == INVALID_SOCKET) {
        std::cerr << "socket() failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        < 0) {
#endif
        return -1;
    }
    if (bind(listen_socket, reinterpret_cast<struct sockaddr*>(&local), sizeof(local))
#ifdef _WIN32
        == SOCKET_ERROR) {
        std::cerr << "bind() failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        < 0) {
#endif
        std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
        exit(-1);
    }
    if (listen(listen_socket, 50)
#ifdef _WIN32
        == SOCKET_ERROR) {
        std::cerr << "listen() failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        < 0) {
#endif
        std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
        exit(-1);
    }
    int threadCnt = 0;
    struct sockaddr_in listenAddr;
    socklen_t listenLen = static_cast<socklen_t>(sizeof(listenAddr));
    getsockname(listen_socket, reinterpret_cast<struct sockaddr*>(&listenAddr), &listenLen);
    fprintf(stdout, "localhost listening [%s:%d].\n", inet_ntoa(listenAddr.sin_addr), srvPort);
#if defined _SYS_WAIT_H || defined _SYS_WAIT_H_
    signal(SIGCHLD, &func_waitpid);
#endif
    do {
        struct sockaddr_in fromAddr = {};
#if (!defined SOCK_CONN_TEST) || (defined SOCK_CONN_TEST)
        type_len addrlen = static_cast<type_len>(sizeof(fromAddr));
        std::cout << "sock " << std::ios::fixed << " len=" << std::ios::hex << addrlen << std::ios::unitbuf << std::endl;
#endif
#ifdef SOCK_CONN_TEST
        type_socket test_socket = accept(listen_socket, (struct sockaddr*)&fromAddr, &addrlen);
        if (test_socket
#ifdef _WIN32
            == INVALID_SOCKET) {
            std::cerr << "Call accept() failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
#else
            < 0) {
#endif // use addrlen
            std::cerr << "ERROR(" << errno << "," << addrlen << "): " << strerror(errno) << std::endl;
            return -1;
        } else {
            char IPdotDec[16];
#ifdef _WIN32
            memcpy(IPdotDec, inet_ntoa(form.sin_addr), 16);
#else
            inet_ntop(AF_INET, (void*)&form.sin_addr, IPdotDec, 16);
#endif
            fprintf(stdout, "Accept [%s] success.\n", IPdotDec);
        }
#ifdef _DEBUG
        fprintf(stdout, ">>> Socket test-%d: val=%d\n", threadCnt, test_socket);
#endif
        closesocket(test_socket);
#endif
#ifdef THREAD_PER_CONN
        if (THREAD_NUM != 0 && threadCnt == THREAD_NUM)
            break;
#endif
#ifdef _WIN32
#ifdef SOCK_CONN_TEST
        _beginthreadex(NULL, 0, (_beginthreadex_proc_type)monitor, (void*)&test_socket, 0, &threadid);
#else
        _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)monitor, nullptr, 0, &threadid);
#endif
#else
#ifdef THREAD_PER_CONN
        pthread_create(&threadid, NULL, monitor, NULL);
#elif !defined SOCK_CONN_TEST || defined SOCK_CONN_TEST
        type_socket msg_socket = accept(listen_socket, (struct sockaddr*)&fromAddr, &addrlen);
        if (msg_socket < 0) {
            std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
            return -1;
        } else {
            int PID = 0;
            if ((PID = fork()) == 0) {
                fprintf(stdout, "Running child(%d) forks from one of [%i] MAIN process.\n", threadCnt, addrlen);
                monitor(&msg_socket);
            } else {
                fprintf(stdout, "Parent: process %d established.\n", PID);
            }
        }
#endif
#endif
        fprintf(stdout, "thrd %d\t0x%08x\n", threadCnt, *(uint32_t*)&threadid);
        threadCnt++;
        SLEEP(99);
    } while (!aim2exit);
    fprintf(stdout, ">>> Executing thread count = %d.\n", threadCnt);
#ifdef THREAD_PER_CONN
    while (true)
        SLEEP(9);
#endif
    return 0;
};

template<typename T> int set_n_get_mem(T * shmem, int ndx, int rw)
{
    int shmid = -1;
    if (ndx > MAX_ACTIVE)
        return rw;
#if defined _SYS_SHM_H || defined _SYS_SHM_H_
    pthread_mutex_trylock(&mutex_allow);
    T* share = NULL;
    if ((shmid = shmget(IPC_KEY, (MAX_ACTIVE * sizeof(T)), (0666 | IPC_FLAG))) < 0) {
        if (EEXIST == errno) {
            shmid = shmget(IPC_KEY, (MAX_ACTIVE * sizeof(T)), 0666);
            share = reinterpret_cast<T*>(shmat(shmid, NULL, 0));
        } else {
            fprintf(stderr, "fail to shmget.\n");
            exit(-1);
        }
    } else {
        share = reinterpret_cast<T*>(shmat(shmid, NULL, 0));
    }
    if (rw >= 1) {
        memmove(share + ndx * sizeof(T), shmem, sizeof(T));
    } else if (rw == 0) {
        memmove(shmem, (char*)share + (ndx * sizeof(T)), sizeof(T));
    } else if (rw + 1 == 0) {
        memset(share + ndx * sizeof(T), 0, sizeof(T));
    } else {
        if (shmdt(share) == -1) {
            fprintf(stderr, "shmdt: detach segment fail.\n");
        }
        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            fprintf(stderr, "shmctl(IPC_RMID) fail.\n");
        }
    }
    pthread_mutex_unlock(&mutex_allow);
#endif
    return shmid;
}
void func_waitpid(int signo)
{
#if defined _SYS_WAIT_H || defined _SYS_WAIT_H_
    int stat;
    pid_t pid;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        char msg[64];
        type_socket sock = -1;
        if (g_filedes[1] > 0)
            close(g_filedes[1]);
        if (g_filedes[0] <= 0) {
            fprintf(stderr, "Can't read socket from filedes[0][%d]: %s\n", g_filedes[0], strerror(errno));
            break;
        }
        ssize_t len = read(g_filedes[0], &sock, sizeof(sock));
        if (len < 0 || len > sizeof(sock)) {
            fprintf(stderr, "Beyond filed size: %zd\n", len);
            break;
        }
        if (sock > 0) {
            memset(msg + 1, 0xf, 1);
            snprintf(msg + 2, 8, "%x", NE_VAL(-1));
            memcpy(msg + 8, "Fail: something wrong with peer !!!", 36);
            ssize_t val = send(sock, msg, 64, 0);
            fprintf(stderr, "Error(%zd) message to client - %s.\n", val, msg + 8);
            memset(msg, 0, 64);
        }
        snprintf(msg, 64, "Signal(%d): child process %d exit just now, sock=%d.\n", signo, pid, sock);
        write(STDERR_FILENO, msg, sizeof(msg));
    }
#endif
}
int queryNetworkParams(const char* username, Network & network, const int max)
{
    struct ONLINE active[MAX_ACTIVE];
    memset(active, 0, sizeof(active));
    for (int c = 0; c < max; c++) {
        set_n_get_mem(&active[c], c);
        if (memcmp(active[c].user, username, FiledSize) == 0) {
            memcpy((char*)network.ip, (char*)active[c].netwk.ip, INET_ADDRSTRLEN);
            network.port = active[c].netwk.port;
            network.socket = active[c].netwk.socket;
            return 0;
        }
    }
    return -1;
}
//for Windows
Network queryNetworkParams(int uid)
{
    Network network;
    memcpy((char*)network.ip, (char*)users[uid].netwk.ip, INET_ADDRSTRLEN);
    network.socket = users[uid].netwk.socket;
    network.port = users[uid].netwk.port;
    return network;
}
void wait(unsigned int tms)
{
    struct timeval time;
    time.tv_sec = tms / 1000;
    time.tv_usec = tms % 1000 * 1000;
    select(0, NULL, NULL, NULL, &time);
}
type_thread_func commands(void* arg)
{
    char options[FiledSize], name[FiledSize];
    do {
        scanf("%23s", reinterpret_cast<char*>(&options));
        if (strcmp(options, "quit") == 0) {
            save_accnt();
            closesocket(listen_socket);
            fprintf(stdout, "Saved accounts to file \"%s\".\n", ACC_REC);
#ifdef _WIN32
            WSACleanup();
            DeleteCriticalSection(&mutex_allow);
#else
            pthread_mutex_destroy(&mutex_allow);
#endif
            aim2exit = 1;
            SLEEP(2);
            exit(0);
        }
        if (strcmp(options, "kick") == 0) {
            fprintf(stdout, "Kick whom out?\n");
            scanf("%23s", reinterpret_cast<char*>(&name));
            int rtn = user_is_line(name);
            if (!(rtn == -1)) {
#if !defined _WIN32
                struct ONLINE active[MAX_ACTIVE];
                memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
                for (int i = 0; i < MAX_ACTIVE; i++) {
                    set_n_get_mem(&active[i], i);
                }
#endif
                closesocket(active[rtn].netwk.socket);
                fprintf(stdout, "User %s kicked out!\n", name);
            }
        }
        if (strcmp(options, "cls") == 0) {
            system("CLS");
        }
        SLEEP(99);
    } while (!(aim2exit));
    return NULL;
}
//save accounts to local file.
int save_accnt()
{
    flush_all();
    FILE* dump = fopen(ACC_REC, "w");
    if (dump == nullptr)
        return -1;
    fwrite(users, sizeof(USER), MAX_USERS, dump);
    fwrite(zones, sizeof(zone_st), MAX_ZONES, dump);
    if (fclose(dump) != 0)
        return -2;
    flush_all();
    return 0;
}
//load accounts from file system.
int load_accnt()
{
    flush_all();
    FILE* dump = fopen(ACC_REC, "r");
    if (dump == nullptr)
        return -1;
    else {
        fread(users, sizeof(USER), MAX_USERS, dump);
        fread(zones, sizeof(zone_st), MAX_ZONES, dump);
        fclose(dump);
        return 0;
    }
    flush_all();
}
int user_auth(char usr[FiledSize], char psw[FiledSize])
{
    char* n = usr, * p = psw;
    if (n == nullptr)
        return -1;
    for (auto& user : users) {
        if ((strcmp(n, user.usr) == 0) && (strcmp(p, user.psw) == 0)) {
            if (user_is_line(n) == -1) {
                return 1; //success
            } else
                return 0; //pass
        } else
            continue;
    }
    //wrong param
    return -2;
}
int new_user(char usr[FiledSize], char psw[FiledSize])
{
    if (usr[0] == '\0')
        return -4;
    if (0 <= get_user_seq(usr)) {
        return -3;
    }
    char* n = usr;
    char* p = psw;
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(n, users[i].usr) == 0)
            return i;
    }
    for (auto& user : users) {
        if (user.usr[0] == '\0') {
            memcpy(user.usr, n, 24);
            memcpy(user.psw, p, 24);
            memcpy(user.intro, "intro weren't set.", 20);
            return -1;
        }
    }
    return -2;
}
int user_is_line(char user[FiledSize])
{
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];  // = { 0, "" };
    memset(active, 0, sizeof(struct ONLINE) * MAX_ACTIVE);
#endif
    for (int i = 0; i < MAX_ACTIVE; i++) {
        set_n_get_mem(&active[i], i);
        if (strncmp(user, active[i].user, FiledSize) == 0) {
            return i;
        }
    }
    return -1;
}
int set_user_line(char user[FiledSize], Network & netwk)
{
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];  // = { 0, "" };
    memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
#endif
    for (int i = 0; i < MAX_ACTIVE; i++) {
        set_n_get_mem(&active[i], i);
        if (active[i].user[0] == '\0') {
            memcpy(active[i].user, user, FiledSize);
            memcpy((char*)active[i].netwk.ip, (char*)netwk.ip, INET_ADDRSTRLEN);
            active[i].netwk.port = netwk.port;
            active[i].netwk.socket = netwk.socket;
            set_n_get_mem(&active[i], i, 1);
            return i;
        }
    }
    return 0;
}
int set_user_quit(char user[FiledSize])
{
    char* u = user;
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];  // = { 0, "" };
    memset(active, 0, sizeof(active));
#endif
    for (int i = 0; i < MAX_ACTIVE; i++) {
        set_n_get_mem(&active[i], i);
        if (strcmp(u, active[i].user) == 0) {
            memset(active[i].user, 0, FiledSize);
            memset((char*)active[i].netwk.ip, 0, INET_ADDRSTRLEN);
            active[i].netwk.socket = 0;
            active[i].netwk.port = 0;
            set_n_get_mem(&active[i], i, -1);
            break;
        }
    }
    return 0;
}
int set_user_peer(const char user[FiledSize], const char ip[INET_ADDRSTRLEN], const int port, type_socket sock)
{
    const char* u = user;
    if (ip[0] == '\0')
        return -1;
    else {
        int m = 0;
        for (int i = 0; i < INET_ADDRSTRLEN; i++) {
            if (ip[i] == 0x2e)
                m++;
        }
        if (m != 3)
            return -2;
    }
    if (port > 65535 || port <= 0)
        return -3;
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(u, users[i].usr) == 0) {
            _CAS(&users[i].netwk.ip[0], *users[i].netwk.ip, *ip);
            users[i].netwk.port = port;
            users[i].netwk.socket = sock;
            return 0;
        }
    }
    return -4;
}
int get_user_seq(char user[FiledSize])
{
    char* u = user;
    if (u[0] == '\0')
        return -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(u, users[i].usr) == 0)
            return i;
    }
    return -2;
}
int find_zone(unsigned char basis[FiledSize])
{
    char* x = reinterpret_cast<char*>(basis);
    if (x[0] == '\0')
        return -1;
    for (int i = 0; i < MAX_ZONES; i++) {
        if (strcmp(zones[i].zone.name, x) == 0)
            return i;
        for (int j = 0; j < MAX_MEMBERS_PER_GROUP; j++) {
            if (strcmp(zones[i].zone.members[j], x) == 0)
                return i;
        }
    }
    return -2;
}
int host_zone(USER & user)
{
    char* host = reinterpret_cast<char*>(user.host);
    unsigned char* brf = user.host + FiledSize;
    char* chief = user.usr;
    for (int i = 0; i < MAX_ZONES; i++) {
        if (strcmp(zones[i].zone.name, host) == 0) {
            return i - MAX_ZONES - 3;
        }
        if (zones[i].zone.name[0] == '\0') {
            memcpy(zones[i].zone.name, host, 24);
            memcpy(zones[i].zone.brief, reinterpret_cast<char*>(brf), 24);
            memcpy(zones[i].zone.chief, chief, 24);
            memcpy(zones[i].zone.members[0], chief, 24);
            return i;
        }
    }
    return -2;
}
int join_zone(int at, char usr[FiledSize], char zone[FiledSize], char* cert)
{
    int i;
    char* m = usr, * z = zone;
    if (at < 0 || m == nullptr)
        return -1;
    if (zones[at].zone.members[MAX_MEMBERS_PER_GROUP - 1][0] != '\0')
        return -2;
    for (i = 0; i < MAX_MEMBERS_PER_GROUP; i++) {
        if (strcmp(zones[at].zone.members[i], m) == 0)
            return -3;
    }
    for (i = 0; i < MAX_MEMBERS_PER_GROUP; i++) {
        if (zones[at].zone.members[i][0] == '\0') {
            if (strcmp(zones[at].zone.name, z) == 0) {
                memcpy(zones[at].zone.members[i], m, 24);
                return i;
            } else {
                continue;
            }
        }
    }
    return -4;
}
int exit_zone(int at, char usr[FiledSize])
{
    char* m = usr;
    if (at < 0)
        return -1;
    for (int i = 0; i < MAX_MEMBERS_PER_GROUP; i++) {
        if (strcmp(zones[at].zone.members[i], m) == 0) {
            memset(zones[at].zone.members[i], 0, FiledSize);
            return i;
        }
    }
    return -2;
}
int free_zone(int at, char host[FiledSize])
{
    if (at < 0)
        return -1;
    char* h = host;
    zone_st* zo = &zones[at].zone;
    if (strcmp(zo->chief, h) == 0) {
        memset(zo, 0, sizeof(zone_st));
        return 0;
    } else return -2;
}
