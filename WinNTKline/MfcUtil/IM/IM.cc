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
#endif
#define NEVAL(a) (((~((a) & 0x0f)) | ((a) & 0xf0)) & 0xff)
#define DEFAULT_PORT 8877
#define IPCKEY 0x520905
#define IPCFLAG IPC_CREAT|IPC_EXCL //|SHM_R|SHM_W
#define THREAD_NUM 20
#define ACC_REC "acnts"
#define MAX_ACTIVE 30
#define MAX_USERS 99
#define MAX_ZONES 10
#define MAX_MENBERS_PER_GROUP 11
//using namespace std;
//C++11 std与socket.h中bind函数冲突
#ifdef _WIN32
typedef int type_len;
typedef SOCKET type_socket;
#ifndef pthread_t
typedef unsigned int pthread_t;
#endif
typedef unsigned int type_thread_func;
#define flush_all() _flushall()
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS
#define SLEEP(t) Sleep((DWORD)t);
#else
typedef int type_socket;
typedef socklen_t type_len;
typedef void *type_thread_func;
#define flush_all() fflush(stdin)
#define closesocket(socket) close(socket)
#define SLEEP(t) usleep((int)1010.10f*(t));
pthread_mutexattr_t attr;
#endif

#define __ "./"
#define _0_ "_0"
#define IMAGE_BLOB "image"
#define GET_IMG_EXE "v4l2.exe"

#ifdef _WIN32
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
sendallow;

int aim2exit = 0;
type_socket listen_socket;
volatile type_socket current_socket = 0;
static unsigned int g_threadNo_ = 0;

struct Network {
    volatile unsigned char ip[INET_ADDRSTRLEN];
    volatile unsigned int port;
    volatile type_socket socket;
};

struct ONLINE {
    char user[24];
    Network netwk;
}
#ifdef _WIN32
active[MAX_ACTIVE]
#endif // _WIN32
;

struct user_clazz {
    char usr[24];
    char psw[24];
    Network netwk;
    unsigned char hgrp[24];
    char intro[24];
}users[MAX_USERS];

struct member
{
    user_clazz user[MAX_MENBERS_PER_GROUP];

    member()
    {
        memset(this, 0, sizeof(*this));
    }
};

typedef struct user_mesg {
    unsigned char rsv;
    unsigned char uiCmdMsg;
    unsigned char rtn[2];
    unsigned char chk[4];
    char usr[24];
    union {
        char psw[24];
        char TOKEN[24];
        char peerIp[24];
    };
    union {
        char peer[24];
        // peer port
        char port[24];
        char sign[24];
        char npsw[24];
        // zone to user
        unsigned char host[24];
        unsigned char join[24];
        unsigned char seek[24];
    };
    class PeerStruct
    {
        unsigned char rsv[2];
    public:
        unsigned char cmd[2];
        unsigned char val[4];
        char msg[16];
    } peer_mesg;
    char status[8];
} USER;

struct zone_clazz {
    char name[24];
    char cert[24];
    char members[MAX_MENBERS_PER_GROUP][24];
    unsigned int usrCnt;
    char chief[24];
    char brief[24];
};

struct zone_file {
    zone_clazz zone;
    zone_file()
    {
        memset(this, 0, sizeof(*this));
    }
    unsigned int getSize() const { return sizeof(*this) + zone.usrCnt * sizeof(member); }
}zones[MAX_ZONES];


void pipesig_handler(int s) {
#ifdef _UNISTD_H
    write(STDERR_FILENO, "Signal: caught a SIGPIPE!\n", 27);
    exit(0);
#endif
}

int inst_mesg(int argc, char* argv[]);
template<typename T> int set_n_get_mem(T* shmem, int ndx = 0, int rw = 0);
void func_waitpid(int signo);
int queryNetworkParams(const char* username, Network& network, const int max = MAX_ACTIVE);
Network queryNetworkParams(int uid);
int user_auth(char usr[24], char psw[24]);
int new_user(char usr[24], char psw[24]);
int user_is_line(char user[24]);
int set_user_line(char user[24], Network& netwk);
int set_user_peer(const char user[24], const char ip[INET_ADDRSTRLEN], const int port, type_socket sock);
int set_user_quit(char user[24]);
int get_user_seq(char user[24]);
int save_acnt();
int load_acnt();
int find_zone(unsigned char basis[24]);
int host_zone(USER &user);
int join_zone(int at, char usr[24], char zone[24], char *cert = NULL);
int exit_zone(int at, char usr[24]);
int free_zone(int at, char host[24]);

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
                if (&active != nullptr && active[c].user[0] != '\0') {
                    fprintf(stdout, "\t%d\t%s\t%s\t%d\t%d\n", c + 1,
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
    inst_mesg(1, { nullptr });
#else
    if (fork() == 0)
    {
        signal(SIGPIPE, pipesig_handler);
        inst_mesg(argc, argv);
    }
#endif
    return 0;
}

type_thread_func monite(void *arg)
{
    static USER user;
    int c, flg, cur = 0;
    int qq = 0, loggedin = 0;
    int valrtn;
    char sd_bufs[256], rcv_txt[256];
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];
    memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
#endif
    type_socket rcv_sock = 0;
    type_socket *sock = reinterpret_cast<type_socket*>(arg);
    char ipAddr[INET_ADDRSTRLEN];
    struct sockaddr_in peerAddr;
    socklen_t peerLen = static_cast<socklen_t>(sizeof(peerAddr));
    if (arg && -1 != long(sock) && int(*sock) != 0) {
        rcv_sock = *sock;
    } else {
#if (!defined THREAD_PER_CONN) && ((!defined _WIN32 ) || (defined SOCK_CONN_TEST))
        std::cerr << "ERROR: socket contra-valid([" << sock << "]" << *sock << ") " << strerror(errno) << std::endl;
        exit(-1);
#else
        struct sockaddr_in sin;
        type_len len = static_cast<type_len>(sizeof(sin));
        rcv_sock = accept(listen_socket, reinterpret_cast<struct sockaddr*>(&sin), &len);
#endif
    }
    getpeername(rcv_sock, reinterpret_cast<struct sockaddr *>(&peerAddr), &peerLen);
    const char* IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
    const int PORT = ntohs(peerAddr.sin_port);
    time_t t;
    struct tm * lt;
    time(&t);
    lt = localtime(&t);
    g_threadNo_++;
    fprintf(stdout, "accepted peer(%d) address [%s:%d] (@ %d/%d/%d %d:%d:%d)\n", g_threadNo_, IP, PORT, lt->tm_year + 1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
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
        std::cerr << "accept() error(" << errno << "): " << strerror(errno) << std::endl;
        return type_thread_func(-1);
    };
    fprintf(stdout, "socket monite: %d; waiting for massage.\n", rcv_sock);
    do {
        memset(sd_bufs, 0, 256);
        memset(rcv_txt, 0, 256);
        memset(&user, 0, sizeof(user));
#ifdef _WIN32
        int
#else
        socklen_t
#endif
            lol = sizeof(int);
        int val;
        current_socket = rcv_sock;
        if (getsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&val), &lol) == 0) {
            if (cur > 0)
                fprintf(stderr, "### Connect--%d status changed, socket: %s.\n", cur, strerror(errno));
            if (errno != 0) {
                set_user_quit(user.usr);
#if defined _WIN32
                ;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                goto comm_err1;
#endif
            }
            cur++;
        }
        flg = recv(rcv_sock, rcv_txt, 256, 0);
        if (qq != flg && flg != 0)
        {
            if (flg <= -1 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                set_user_quit(user.usr);
                fprintf(stderr, "----------------------------------------------------------------\
                    \n### Client socket [%d] closed by itself just now.\n", rcv_sock);
                closesocket(rcv_sock);
#ifdef _WIN32
                break;
#else
                exit(0);
#endif
            } else {
#ifdef _DEBUG
                memcpy(&user, rcv_txt, sizeof(user));
                fprintf(stdout, "----------------------------------------------------------------\
                \n>>> 1-RCV [%0x,%0x]: %s, %d, %d\n", user.uiCmdMsg, static_cast<unsigned>(*user.chk), user.usr, flg, THREAD_NUM - g_threadNo_);
                for (c = 0; c < flg; c++)
                {
                    if (c > 0 && c % 32 == 0)
                        fprintf(stdout, "\n");
                    fprintf(stdout, "%02x ", static_cast<unsigned char>(rcv_txt[c]));
                }
                fprintf(stdout, "\n");
#endif
            }
            int snres = -1;
            char userName[24];
            memset(userName, 0, 24);
            memcpy(sd_bufs, &user, 2);
            // rcv_txt: inc--8 bit crc_head, 24 bit username, 24 bit password.
            if ((user.rsv == 0) && (user.uiCmdMsg == 0)) {
                valrtn = new_user(user.usr, user.psw);
                sprintf(sd_bufs + 2, "%x", NEVAL(valrtn + 1));
                if (valrtn == -1) {
                    sprintf(sd_bufs + 8, "New user: %s", user.usr);
                    join_zone(0, user.usr, (char*)("all"));
                    snres = send(rcv_sock, sd_bufs, 48, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                } else if (valrtn == -2) {
                    strcpy((sd_bufs + 8), "Too many users.");
                    snres = send(rcv_sock, sd_bufs, 28, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                } else if (valrtn >= 0) {
                    strcpy((sd_bufs + 8), "User already exists.");
                    snres = send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                } else if (valrtn == -3) {
                    strcpy((sd_bufs + 8), "Same user name exist.");
                    snres = send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                } else if (valrtn == -4) {
                    strcpy((sd_bufs + 8), "User name error.");
                    snres = send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                };
                if (snres < 0) {
                    fprintf(stderr, "### Socket status: %s\n", strerror(errno));
                }
                continue;
                break;
            } else if ((user.rsv == 0) && (user.uiCmdMsg == 0x1)) {
                // user.usr: 8; password: 32.
                valrtn = user_auth(user.usr, user.psw);
                sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                if (valrtn == (loggedin = 1)) {
                    sprintf(sd_bufs + 8, "[%s] logging on successfully.", user.usr);
                    set_user_peer(user.usr, IP, PORT, rcv_sock);
                    Network sock;
                    memcpy((void*)sock.ip, IP, INET_ADDRSTRLEN),
                        sock.port = PORT,
                        sock.socket = rcv_sock;
                    set_user_line(user.usr, sock);
                    memcpy(userName, user.usr, 24);
                    send(rcv_sock, sd_bufs, 64, 0);
                } else if (valrtn == 0) {
                    if (memcmp(user.usr, userName, 24) == 0) {
                        sprintf(sd_bufs + 2, "%x", NEVAL(-3));
                        sprintf(sd_bufs + 8, "Another [%s] is on line.", user.usr);
                        snres = send(rcv_sock, sd_bufs, 64, 0);
                        if (snres < 0) {
                            fprintf(stderr, "### User status: %s\n", strerror(errno));
#if defined _WIN32
                            return NULL;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                            goto comm_err1;
#endif
                        } else
                            fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                        continue;
                    } else {
                        sprintf((sd_bufs + 2), "%02x", 0xe8);
                        sprintf((sd_bufs + 8), "LoggingIn status invalid, will close this socket.");
                        set_user_quit(user.usr);
                        send(rcv_sock, sd_bufs, 64, 0);
                        closesocket(rcv_sock);
                        fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                        loggedin = 0;
                        continue;
                    }
                } else if (valrtn < 0) {
                    sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                    strcpy((sd_bufs + 8), "Error: check username/password again.");
                    send(rcv_sock, sd_bufs, 48, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                } else {
                    fprintf(stdout, ">>> Unknown Error!\n");
                    continue;
                };
            } else if ((user.rsv == 0) && (user.uiCmdMsg == 0x3)) {
                sprintf(sd_bufs + 2, "%x", NEVAL(-1));
                strcpy((sd_bufs + 8), "Warning: user hasn't logged on yet.");
                send(rcv_sock, sd_bufs, 48, 0);
                fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                continue;
            } else {
                strcpy((sd_bufs + 8), "Warning: please Register(0) or Login(1) at first.");
                sprintf(sd_bufs + 2, "%x", NEVAL(-2));
                send(rcv_sock, sd_bufs, 56, 0);
                fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                continue;
            }
#ifdef _DEBUG
            fprintf(stdout, ">>> 1-MSG [%0x,%0x]: ", sd_bufs[0], sd_bufs[1]);
            for (c = 2; c < 64; c++)
                fprintf(stdout, "%c", static_cast<unsigned char>(sd_bufs[c]));
            fprintf(stdout, "\n");
#endif
            // set cur qq as last flg;
            qq = flg;
            while (loggedin) {
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
                flg = recv(rcv_sock, rcv_txt, 256, 0);
                if (flg < 0 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                    set_user_quit(userName);
                    fprintf(stderr, "### Lost connection with *[%s]: %s(%d)\n", userName, strerror(errno), flg);
#if defined _WIN32 || !defined THREAD_PER_CONN || !defined SOCK_CONN_TEST
                    return NULL;
#else
                    loggedin = 0;
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
                            fprintf(stderr, "### Request param invalid.\n");
                            goto comm_err0;
                        }
                    }
                    memcpy(&user, rcv_txt, flg);
                }
                if (flg > 0) {
#ifdef _DEBUG
                    fprintf(stdout, "----------------------------------------------------------------\
                    \n>>> 2-RCV [%d,%x]: %s, %d\n", user.uiCmdMsg, static_cast<unsigned>(*user.chk), user.usr, flg);
                    for (c = 0; c < flg; c++)
                    {
                        if (c > 0 && c % 32 == 0)
                            fprintf(stdout, "\n");
                        fprintf(stdout, "%02x ", static_cast<unsigned char>(rcv_txt[c]));
                    }
                    fprintf(stdout, "\n");
#endif
                }
                unsigned int sndlen = 256;
                memset(sd_bufs, 0, sndlen);
                if (user.rsv == 0)
                {
                    switch (sd_bufs[1] = user.uiCmdMsg)
                    {
                    case 0x01:
                        strcpy((sd_bufs + 8), "User has already on-line.");
                        sndlen = 48;
                        break;
                    case 0x2:
                        char susr[sizeof(user)];
                        sprintf(susr, "User: %s(--%s--);", user.usr, user.sign);
                        snprintf((sd_bufs + 8), strlen(susr) + 1, susr);
                        sndlen = 8 + strlen(susr) + 1;
                        break;
                    case 0x3:
                    {
                        flg = loggedin = 0;
                        set_user_quit(userName);
                        sprintf(sd_bufs + 8, "[%s] has logout.", userName);
                        send(rcv_sock, sd_bufs, 48, 0);
                        fprintf(stderr, "### [%0x, %x]: %s\n", sd_bufs[1], static_cast<unsigned>(*user.chk), sd_bufs + 8);
                        continue;
                    }
                    case 0x4:
                    {
                        valrtn = get_user_seq(user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (1 == user_auth(user.usr, user.psw) && user.npsw != nullptr)
                            strcpy(users[valrtn].psw, user.npsw);
                        else
                            strcpy((sd_bufs + 8), "Change password failure: user auth error.");
                        sndlen = 56;
                    } break;
                    case 0x5:
                    {
                        strcpy((sd_bufs + 8), "Users on-line list:\n");
                        for (c = 0; c < MAX_ACTIVE; c++) {
                            set_n_get_mem(&active[c], c);
                            if (strlen(active[c].user) > 0) {
                                sprintf(sd_bufs + 2, "%x", c);
                                strcpy((sd_bufs + 8 * (c + 4)), active[c].user);
                                if ((sd_bufs + 8 * (c + 4) + 24) == (char*)'\0') {
                                    memset(sd_bufs + 8 * (c + 4) + 24, '\t', 1);
                                }
                            } else if (active[c].user[0] == '\0')
                                break;
                        };
                        sndlen = 8 * (c + 4 + 4);
                    } break;
                    case 0x6:
                    {
                        unsigned int uiIP = 0;
                        strcpy((sd_bufs + 8), user.usr);
                        valrtn = get_user_seq(user.peer);
                        if (valrtn >= 0) {
                            if (user_is_line(user.peer) >= 0) {
                                fprintf(stdout, "``` '%s' wants to P2P with '%s'\n", user.usr, user.peer);
                                Network p2pnet = queryNetworkParams(valrtn);
#if  !defined _WIN32
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
                                sprintf((sd_bufs + 32), "%d", uiIP);
                                sprintf((sd_bufs + 56), "%d", p2pnet.port);
                                if (memcmp(user.chk, "P2P", 4) == 0) { // recieve a hole digging message
                                    fprintf(stdout, "'%s' is communicating with '%s' via P2P.\n", user.usr, user.peer);
                                    sprintf(sd_bufs + 2, "%x", NEVAL(0));
                                    strcpy((sd_bufs + 4), "P2P");
                                    sprintf((sd_bufs + 64), "%s request a hole.", user.usr);
                                    if (-1 != send(p2pnet.socket, sd_bufs, 108, 0)) {
                                        fprintf(stdout, "Success send command to %s.\n", user.peer);
                                    } else {
                                        memset(sd_bufs + 8, 0, 248);
                                        sprintf(sd_bufs + 2, "%x", NEVAL(-1));
                                        sprintf((sd_bufs + 32), "fail send command to %s.", user.peer);
                                        send(rcv_sock, sd_bufs, sndlen, 0);
                                        fprintf(stdout, "Got failure trans command to %s:%d (%s).\n", p2pnet.ip, p2pnet.port, strerror(errno));
                                    }
                                    break;
                                }
                                // p2p_req2usr
                            } else {
                                valrtn = -2;
                                strcpy((sd_bufs + 32), "P2P peer user offline!");
                            }
                        } else {
                            valrtn = -3;
                            sprintf((sd_bufs + 32), "Get user network: No such user(%s)!", user.peer);
                        }
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        unsigned char *val = reinterpret_cast<unsigned char *>(&uiIP);
                        fprintf(stdout, ">>> get client peer [%u.%u.%u.%u:%s]\n", val[3], val[2], val[1], val[0], sd_bufs + 56);
                        sndlen = 96;
                    } break;
                    case 0x7:
                    {
                        strcpy((sd_bufs + 8), user.usr);
                        if (memcmp(user.chk, "NDT", 4) == 0) {
                            fprintf(stdout, "'%s' is communicating with '%s' via NDT.\n", user.usr, user.peer);
                        } else {
                            strcpy((sd_bufs + 32), "Check message error for Network Data Translation.");
                            sndlen = 88;
                            break;
                        }
                        valrtn = get_user_seq(user.peer);
                        if (valrtn >= 0) {
                            if (user_is_line(user.peer) >= 0) {
                                Network ndtnet = queryNetworkParams(valrtn);
#if  !defined _WIN32
                                queryNetworkParams(user.peer, ndtnet);
#endif
                                if (ndtnet.ip == nullptr || ndtnet.port <= 0) {
                                    strcpy((sd_bufs + 32), "Peer user ip or port value error.");
                                    sndlen = 72;
                                } else {
                                    char random = (char)(rand() % 255 + '\1');
                                    char ndtmsg[32];
                                    memset(ndtmsg, 0, 32);
                                    memcpy(ndtmsg, &user, 4);
                                    sprintf(ndtmsg + 4, "%c", random);
                                    strcpy((char*)&user.status, "200");
                                    memcpy(ndtmsg + 8, &user.peer_mesg.msg, 24);
                                    if (-1 != send(ndtnet.socket, ndtmsg, 32, 0)) {
                                        if (ndtnet.socket == rcv_sock) {
                                            strcpy((sd_bufs + 32), "Socket descriptor conflicts!");
                                            sndlen = 64;
                                        } else {
                                            sprintf(sd_bufs + 32, "[%c] NDT success to %s.", random, user.peer);
                                            sndlen = 80;
                                        }
                                    } else {
                                        struct stat sock_stat;
                                        if (EBADF == fstat(ndtnet.socket, &sock_stat)) {
                                            fprintf(stdout, "Error: socket descriptor - %d.\n", ndtnet.socket);
                                        } else {
                                            fprintf(stdout, "Error: %s(%I32d).\n", strerror(errno), ndtnet.socket);
                                        }
                                        strcpy((sd_bufs + 32), "Got failure while senting a message.");
                                        sndlen = 80;
                                    }
                                }
                            } else {
                                snprintf((sd_bufs + 32), 23, "NDT Peer User Offline!");
                                memset(sd_bufs + 2, NEVAL(-2), 1);
                                sndlen = 72;
                            }
                        } else {
                            sprintf((sd_bufs + 32), "Get user network: No such user(%s)!", user.peer);
                            sprintf((sd_bufs + 2), "%x", NEVAL(-3));
                            sndlen = 96;
                        }
                    } break;
                    case 0x8:
                    {
#if !defined _WIN32
                        char *mesg = NULL;
                        if (vfork() == 0)
                        {
#ifdef RASPI
#undef GET_IMG_EXE
#define GET_IMG_EXE "raspistill"
#undef __ 
#undef _0_
#define __ ""
#define _0_ ""
                            char *const agv[] = { (char*)GET_IMG_EXE, (char*)"-o", (char*)IMAGE_BLOB, (char *)(0) };
#else
                            char *const agv[] = { (char*)GET_IMG_EXE, (char *)(0) };
#endif
                            valrtn = execvp(__ GET_IMG_EXE, agv);
                            mesg = strerror(errno);
                            sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                            strcpy((sd_bufs + 8), mesg);
                            sndlen = 8 + strlen(mesg) + 1;
                            fprintf(stdout, "Exec [ %s ] with execvp fail, %s.\n", __ GET_IMG_EXE, mesg);
                        } else {
                            wait(&valrtn);
                            if (errno != ECHILD) {
                                mesg = strerror(errno);
                            } else {
                                if (access(IMAGE_BLOB _0_, 0) == 0) {
                                    mesg = (char*)"success";
                                } else {
                                    mesg = (char*)"Not save image file";
                                }
                            }
                            sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                            strcpy((sd_bufs + 8), mesg);
                            sndlen = 8 + strlen(mesg) + 1;
                            fprintf(stdout, "Make image via '%s': %s.\n", __ GET_IMG_EXE, mesg);
                        }
#else
                        char *mesg = "OS don't support v4l.";
                        sndlen = 32;
                        strcpy((sd_bufs + 8), mesg);
                        fprintf(stdout, "%s\n", mesg);
#endif
                    } break;
                    case 0x9:
                    {
                        FILE * file = fopen(IMAGE_BLOB _0_, "rb");
                        if (file == NULL)
                        {
                            sprintf((sd_bufs + 8), "Fail read image file \"%s\".", IMAGE_BLOB);
                            fprintf(stdout, "%s\n", sd_bufs + 8);
                            sndlen = 8 + strlen(IMAGE_BLOB) + 27;
                            break;
                        }
                        fseek(file, 0, SEEK_END);
                        long lSize = ftell(file);
                        rewind(file);
                        int bytes = lSize / sizeof(unsigned char);
                        if (bytes > 2097152)
                            bytes = 4096;
                        memset(sd_bufs + 8, bytes, 6);
                        unsigned char *pos = (unsigned char*)malloc(sizeof(unsigned char)*bytes);
                        if (pos == NULL)
                        {
                            sprintf((sd_bufs + 8), "Fail malloc for \"%s\".", IMAGE_BLOB);
                            fprintf(stdout, "%s\n", sd_bufs + 8);
                            sndlen = 8 + strlen(IMAGE_BLOB) + 24;
                            break;
                        }
                        int slice = 0;
                        while (int rcsz = fread(pos, sizeof(unsigned char), bytes, file) != 0 && !feof(file)) {
                            fprintf(stdout, "        "
                                "File \"%s\" size = %d, item = %d, slice = %d.\r", IMAGE_BLOB, bytes, rcsz, slice);
                            sprintf((sd_bufs + 14), "%04d", slice);
                            memset(sd_bufs + 1, user.uiCmdMsg, 1);
                            volatile int cur = 0;
                            for (int i = 0; i <= bytes; ++i, ++cur) {
                                if (((i > 0) && (i % 224 == 0)) || (i == bytes)) {
                                    sprintf((sd_bufs + 22), "%04d", i);
                                    send(rcv_sock, sd_bufs, 256, 0);
                                    memset(sd_bufs + 32, 0, 224);
                                    SLEEP(1.0 / 10000);
                                    cur = 0;
                                }
                                memset(sd_bufs + 32 + cur, pos[i], 1);
                                fprintf(stdout, /*"\r%*c*/"\r%.02f%% ", /*7, ' ',*/ i*100.f / bytes);
                            }
                            slice++;
                        }
                        fprintf(stdout, "\r\n");
                        free(pos);
                        fclose(file);
                    }
                    break;
                    case 10:
                    {
                        strcpy((sd_bufs + 8), "Active zone list: \n");
                        for (c = 0; c < MAX_ZONES; c++) {
                            if (strlen(zones[c].zone.name) >> 0) {
                                sprintf(sd_bufs + 2, "%x", c);
                                strcpy((sd_bufs + 8 * (c + 4)), zones[c].zone.name);
                            };
                            if (zones[c].zone.name[0] == '\0')
                                break;
                        };
                        sndlen = 8 * (c + 4 + 3);
                    } break;
                    case 11:
                    {
                        valrtn = find_zone(user.seek);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -2) {
                            sprintf((sd_bufs + 8), "No such [%s] zone.", user.seek);
                            sndlen = 56;
                        } else {
                            memset(sd_bufs + 2, 0, 2);
                            strcpy((sd_bufs + 8), "Member(s) of the zone:\n");
                            for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
                                if (strlen(zones[valrtn].zone.members[c]) >> 0) {
                                    strcpy((sd_bufs + 8 * (c + 4)), zones[valrtn].zone.members[c]);
                                };
                            };
                            if (valrtn == -1 || zones[valrtn].zone.members[c][0] == '\0')
                                break;
                            sndlen = 8 * (c + 4 + 4);
                        };
                    } break;
                    case 12:
                    {
                        valrtn = host_zone(user);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -2) {
                            strcpy((sd_bufs + 8), "Host zone rejected.");
                            sndlen = 32;
                        } else if (valrtn < -2) {
                            join_zone((valrtn + MAX_ZONES + 3), user.usr, reinterpret_cast<char*>(user.join));
                            sprintf((sd_bufs + 8), "%s exist, %s joins the zone.", user.host, user.usr);
                            sndlen = 88;
                        } else {
                            sprintf((sd_bufs + 8), "Create zone '%s' as host.", user.host);
                            sndlen = 64;
                        }
                    } break;
                    case 13:
                    {
                        valrtn = join_zone(find_zone(user.join), user.usr, reinterpret_cast<char*>(user.join));
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn < 0) {
                            sndlen = 48;
                            switch (valrtn) {
                            case -1:
                                strcpy((sd_bufs + 8), "Can not find such zone name.");
                                break;
                            case -2:
                                strcpy((sd_bufs + 8), "Limits: zone was full for new members.");
                                break;
                            case -3:
                                strcpy((sd_bufs + 8), "You have already joined this zone.");
                                break;
                            case -4:
                                strcpy((sd_bufs + 8), "Wrong pass token to this zone.");
                                break;
                            default:
                                strcpy((sd_bufs + 8), "Unknown error.");
                                break;
                            }
                        } else {
                            sndlen = 64;
                            sprintf((sd_bufs + 8), "Joining zone (%s) successfully.", user.join);
                        }
                    } break;
                    case 14:
                    {
                        valrtn = exit_zone(find_zone(user.seek), user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        sndlen = 42;
                        if (valrtn >= 0)
                            strcpy((sd_bufs + 8), "Leave zone successfully.");
                        else
                            strcpy((sd_bufs + 8), "You aren't yet in this zone.");
                    } break;
                    case 15:
                    {
                        valrtn = free_zone(find_zone(user.seek), user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        for (int i = 0; i < MAX_ACTIVE; i++)
                            set_n_get_mem(&active[i], i);
                        if (valrtn < 0) {
                            if (-1 != user_is_line(user.usr)) {
                                sprintf(sd_bufs + 3, "%x", -1);
                                strcpy((sd_bufs + 8), user.usr);
                                strcpy((sd_bufs + 32), user.sign);
                                send(rcv_sock, sd_bufs, 48, 0);
                            };
                        } else {
                            for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
                                if (!strlen(zones[valrtn].zone.members[c]) == 0)
                                {
                                    sprintf(sd_bufs + 3, "%x", -3);
                                    int uil = user_is_line(zones[valrtn].zone.members[c]);
                                    if (!(uil == -1)) {
                                        strcpy((sd_bufs + 8), user.usr);
                                        strcpy((sd_bufs + 32), user.sign);
                                        send(active[uil].netwk.socket, sd_bufs, 48, 0);
                                    }//if uil
                                }//if strlen
                            }//for
                        }//else
                    } break;
                    default: break;
                    }
                    if ((memcmp(user.chk, "P2P", 4) != 0) &&
                        (send(rcv_sock, sd_bufs, sndlen, 0) < 0))
                    {
                        perror("Socket lost");
#if defined _WIN32
                        return NULL;
#elif THREAD_PER_CONN || SOCK_CONN_TEST
                        goto comm_err1;
                        break;
#endif
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
    } while (!loggedin);

comm_err0: {
    for (int i = 0; i < MAX_ACTIVE; i++)
        set_n_get_mem(&active[i], i);
    memset(active[user_is_line(user.usr)].user, 0, 24);
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

type_thread_func commands(void *arg)
{
    char optionstr[24], name[24];
    do {
        scanf("%32s", reinterpret_cast<char*>(&optionstr));
        if (strcmp(optionstr, "quit") == 0) {
            save_acnt();
            closesocket(listen_socket);
            fprintf(stdout, "Saved accounts to file \"%s\".\n", ACC_REC);
#ifdef _WIN32
            WSACleanup();
            DeleteCriticalSection(&sendallow);
#else
            pthread_mutex_destroy(&sendallow);
#endif
            aim2exit = 1;
            SLEEP(2);
            exit(0);
        }
        if (strcmp(optionstr, "kick") == 0) {
            fprintf(stdout, "Kick whom out?\n");
            scanf("%32s", reinterpret_cast<char*>(&name));
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
            };
        };
        if (strcmp(optionstr, "cls") == 0) {
            system("CLS");
        };
        SLEEP(99);
    } while (!(aim2exit));
    return NULL;
};

int inst_mesg(int argc, char * argv[])
{
    int servport = DEFAULT_PORT;
    if (argc == 2 && atoi(argv[1]) != 0) {
        servport = atoi(argv[1]);
    }
    if (!load_acnt()) {
        fprintf(stdout, "accounts load finish from [%s].\n", ACC_REC);
    } else {
        strcpy(users[0].usr, "iv9527");
        strcpy(users[0].psw, "tesT123$");
        strcpy(users[1].usr, "AAAAA");
        strcpy(users[1].psw, "AAAAA");
        strcpy(zones[0].zone.name, "all");
        strcpy(zones[0].zone.brief, "all");
        strcpy(zones[0].zone.members[0], "iv9527");
    }
#ifdef _WIN32
    InitializeCriticalSection(&sendallow);
#else
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(sendallow), &attr);
#endif
    aim2exit = 0;
#ifdef _WIN32
    SetConsoleTitle((
#ifdef _UNICODE
        LPCWSTR
#else
        LPCSTR
#endif
        )"chat server for network design");
    pthread_t threadid;
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
    local.sin_port = htons((uint16_t)servport);
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
    int thdcnt = 0;
    struct sockaddr_in listenAddr;
    socklen_t listenLen = static_cast<socklen_t>(sizeof(listenAddr));
    getsockname(listen_socket, reinterpret_cast<struct sockaddr *>(&listenAddr), &listenLen);
    fprintf(stdout, "localhost listening [%s:%d].\n", inet_ntoa(listenAddr.sin_addr), servport);
#ifdef _SYS_WAIT_H
    signal(SIGCHLD, &func_waitpid);
#endif
    do {
        struct sockaddr_in from;
        type_len folen = static_cast<type_len>(sizeof(from));
#ifdef SOCK_CONN_TEST
        type_socket test_socket = accept(listen_socket, (struct sockaddr*)&from, &folen);
        if (test_socket
#ifdef _WIN32
            == INVALID_SOCKET) {
            std::cerr << "accept() failed error " << WSAGetLastError() << std::endl;
            WSACleanup();
#else
            < 0) {
#endif
            std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
            return -1;
        } else
        {
            char IPdotdec[16];
#ifdef _WIN32
            memcpy(IPdotdec, inet_ntoa(from.sin_addr), 16);
#else
            inet_ntop(AF_INET, (void *)&from.sin_addr, IPdotdec, 16);
#endif
            fprintf(stdout, "accept [%s] success.\n", IPdotdec);
        }
#ifdef _DEBUG
        fprintf(stdout, ">>> Socket test-%d: val=%d\n", thdcnt, test_socket);
#endif
        closesocket(test_socket);
#endif
#ifdef THREAD_PER_CONN
        if (THREAD_NUM != 0 && thdcnt == THREAD_NUM)
            break;
#endif
#ifdef _WIN32
#ifdef SOCK_CONN_TEST
        _beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, (void*)&test_socket, 0, &threadid);
#else
        _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)monite, nullptr, 0, &threadid);
#endif
#else
#ifdef THREAD_PER_CONN
        pthread_t threadid;
        pthread_create(&threadid, NULL, monite, NULL);
#elif !defined SOCK_CONN_TEST
        type_socket msg_socket = accept(listen_socket, (struct sockaddr*)&from, &folen);
        if (msg_socket < 0) {
            std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
            return -1;
        } else {
            int PID = 0;
            if ((PID = fork()) == 0)
            {
                fprintf(stdout, "running child(%d) fork from main process.\n", thdcnt);
                monite(&msg_socket);
            } else
                fprintf(stdout, "parent: process %d established.\n", PID);
        }
#endif
#endif
        thdcnt++;
        SLEEP(99);
        } while (!aim2exit);
        fprintf(stdout, ">>> Executing thread count = %d.\n", thdcnt);
#ifdef THREAD_PER_CONN
        while (true)
            SLEEP(9);
#endif
        return 0;
    }
template<typename T> int set_n_get_mem(T* shmem, int ndx, int rw) {
    int shmids = -1;
    if (ndx > MAX_ACTIVE)
        return rw;
#ifdef _SYS_SHM_H
    pthread_mutex_trylock(&sendallow);
    T *shared;
    if ((shmids = shmget(IPCKEY, MAX_ACTIVE * sizeof(T), 0666 | IPCFLAG) < 0))
    {
        if (EEXIST == errno)
        {
            shmids = shmget(IPCKEY, MAX_ACTIVE * sizeof(T), 0666);
            shared = (T*)shmat(shmids, NULL, 0);
        } else
        {
            fprintf(stderr, "fail to shmget.\n");
            exit(-1);
        }
    } else {
        shared = (T*)shmat(shmids, NULL, 0);
    }
    if (rw >= 1) {
        memmove(shared + ndx * sizeof(T), shmem, sizeof(T));
    } else if (rw == 0) {
        memmove(shmem, shared + ndx * sizeof(T), sizeof(T));
    } else if (rw + 1 == 0) {
        memset(shared + ndx * sizeof(T), 0, sizeof(T));
    } else {
        if (shmdt(shared) == -1)
        {
            fprintf(stderr, "shmdt: detach segment fail.\n");
        }
        if (shmctl(shmids, IPC_RMID, 0) == -1)
        {
            fprintf(stderr, "shmctl(IPC_RMID) fail.\n");
        }
    }
    pthread_mutex_unlock(&sendallow);
#endif
    return shmids;
}
void func_waitpid(int signo) {
#ifdef _SYS_WAIT_H
    int stat;
    pid_t pid;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        static char buf[64];
        sprintf(buf, "Signal(%d): child process %d exit just now.\n", signo, pid);
        write(STDERR_FILENO, buf, strlen(buf));
    }
    return;
#endif
}

int queryNetworkParams(const char * username, Network & network, const int max)
{
    struct ONLINE active[MAX_ACTIVE];
    memset(active, 0, sizeof(active));
    for (int c = 0; c < max; c++) {
        set_n_get_mem(&active[c], c);
        if (memcmp(active[c].user, username, 24) == 0) {
            memcpy((char*)network.ip, (char*)active[c].netwk.ip, INET_ADDRSTRLEN);
            network.port = active[c].netwk.port;
            network.socket = active[c].netwk.socket;
            return 0;
        }
    }
    return -1;
}
// for Windows
Network queryNetworkParams(int uid)
{
    Network network;
    memcpy((char*)network.ip, (char*)users[uid].netwk.ip, INET_ADDRSTRLEN);
    network.socket = users[uid].netwk.socket;
    network.port = users[uid].netwk.port;
    return network;
}

//save accounts to file.
int save_acnt() {
    flush_all();
    FILE *dumpfile = nullptr;
    dumpfile = fopen(ACC_REC, "w");
    if (dumpfile == nullptr)
        return -1;
    fwrite(users, sizeof(USER), MAX_USERS, dumpfile);
    fwrite(zones, sizeof(zone_clazz), MAX_ZONES, dumpfile);
    if (fclose(dumpfile) != 0)
        return -2;
    flush_all();
    return 0;
};
//load file system from disk.
int load_acnt() {
    flush_all();
    FILE* dumpfile = fopen(ACC_REC, "r");
    if (dumpfile == nullptr)
        return -1;
    else {
        fread(users, sizeof(USER), MAX_USERS, dumpfile);
        fread(zones, sizeof(zone_clazz), MAX_ZONES, dumpfile);
        fclose(dumpfile);
        return 0;
    };
    flush_all();
};
int user_auth(char usr[24], char psw[24]) {
    char *n = usr, *p = psw;
    if (n == nullptr)
        return -1;
    for (auto& user : users)
    {
        if ((strcmp(n, user.usr) == 0) && (strcmp(p, user.psw) == 0)) {
            if (user_is_line(n) == -1) {
                return 1;   //success
            } else
                return 0;   //pass
        } else
            continue;
    };
    //wrong param
    return -2;
};
int new_user(char usr[24], char psw[24]) {
    if (usr[0] == '\0')
        return -4;
    if (0 <= get_user_seq(usr)) {
        return -3;
    }
    char *n = usr;
    char *p = psw;
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(n, users[i].usr) == 0)
            return i;
    }
    for (auto& user : users)
    {
        if (user.usr[0] == '\0') {
            strcpy(user.usr, n);
            strcpy(user.psw, p);
            strcpy(user.intro, "intro weren't set.");
            return -1;
        }
    }
    return -2;
};
int user_is_line(char user[24]) {
    char *tomatch = user;
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];  // = { 0, "" };
    memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
#endif
    for (int i = 0; i < MAX_ACTIVE; i++) {
        set_n_get_mem(&active[i], i);
        if (strcmp(tomatch, active[i].user) == 0)
            return i;
    }
    return -1;
};
int set_user_line(char user[24], Network& netwk) {
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];  // = { 0, "" };
    memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
#endif
    for (int i = 0; i < MAX_ACTIVE; i++) {
        set_n_get_mem(&active[i], i);
        if (active[i].user[0] == '\0')
        {
            memcpy(active[i].user, user, 24);
            memcpy((char*)active[i].netwk.ip, (char*)netwk.ip, INET_ADDRSTRLEN);
            active[i].netwk.port = netwk.port;
            active[i].netwk.socket = netwk.socket;
            set_n_get_mem(&active[i], i, 1);
            return i;
        }
    }
    return 0;
};
int set_user_quit(char user[24]) {
    char *u = user;
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];  // = { 0, "" };
    memset(active, 0, sizeof(active));
#endif
    for (int i = 0; i < MAX_ACTIVE; i++) {
        set_n_get_mem(&active[i], i);
        if (strcmp(u, active[i].user) == 0)
        {
            memset(active[i].user, 0, 24);
            memset((char*)active[i].netwk.ip, 0, INET_ADDRSTRLEN);
            active[i].netwk.socket = 0;
            active[i].netwk.port = 0;
            set_n_get_mem(&active[i], i, -1);
            break;
        }
    }
    return 0;
}
int set_user_peer(const char user[24], const char ip[INET_ADDRSTRLEN], const int port, type_socket sock)
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
            strcpy((char*)users[i].netwk.ip, ip);
            users[i].netwk.port = port;
            users[i].netwk.socket = sock;
            return 0;
        }
    }
    return -4;
}
int get_user_seq(char user[24]) {
    char *u = user;
    if (u[0] == '\0')
        return -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(u, users[i].usr) == 0)
            return i;
    };
    return -2;
};
int find_zone(unsigned char basis[24]) {
    char *x = (char*)basis;
    if (x[0] == '\0')
        return -1;
    for (int i = 0; i < MAX_ZONES; i++) {
        if (strcmp(zones[i].zone.name, x) == 0)
            return i;
        for (int j = 0; j < MAX_MENBERS_PER_GROUP; j++) {
            if (strcmp(zones[i].zone.members[j], x) == 0)
                return i;
        }
    };

    return -2;
};
int host_zone(USER &user) {
    char *host = (char*)user.host;
    unsigned char* brf = user.host + 24;
    char* chief = user.usr;
    for (int i = 0; i < MAX_ZONES; i++) {
        if (strcmp(zones[i].zone.name, host) == 0) {
            return i - MAX_ZONES - 3;
        }
        if (strlen(zones[i].zone.name) == 0) {
            strcpy(zones[i].zone.name, host);
            strcpy(zones[i].zone.brief, reinterpret_cast<char*>(brf));
            strcpy(zones[i].zone.chief, chief);
            strcpy(zones[i].zone.members[0], chief);
            return i;
        };
    };
    return -2;
};
int join_zone(int at, char usr[24], char zone[24], char* cert) {
    int i;
    char *m = usr, *z = zone;
    if (at < 0)
        return -1;
    if (zones[at].zone.members[MAX_MENBERS_PER_GROUP - 1][0] != '\0')
        return -2;
    for (i = 0; i < MAX_MENBERS_PER_GROUP; i++) {
        if (strcmp(zones[at].zone.members[i], m) == 0)
            return -3;
    };
    for (i = 0; i < MAX_MENBERS_PER_GROUP; i++) {
        if (strlen(zones[at].zone.members[i]) == 0) {
            if (strcmp(zones[at].zone.name, z) == 0) {
                strcpy(zones[at].zone.members[i], m);
                return i;
            } else {
                continue;
            }
        };
    };
    return -4;
};
int exit_zone(int at, char usr[24]) {
    char *m = usr;
    if (at < 0)
        return -1;
    for (int i = 0; i < MAX_MENBERS_PER_GROUP; i++) {
        if (strcmp(zones[at].zone.members[i], m) == 0) {
            memset(zones[at].zone.members[i], 0, 24);
            return i;
        };
    };
    return -2;
};
int free_zone(int at, char host[24]) {
    if (at < 0)
        return -1;
    char *h = host;
    zone_clazz *zo = &zones[at].zone;
    if (strcmp(zo->chief, host) == 0) {
        memset(zo, 0, sizeof(zone_clazz));
        return 0;
    } else return -2;
};
