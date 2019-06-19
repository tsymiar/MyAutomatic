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
#define MAX_GROUPS 10
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
#define SLEEP(t) Sleep(t);
#else
typedef int type_socket;
typedef socklen_t type_len;
typedef void *type_thread_func;
#define flush_all() fflush(stdin)
#define closesocket(socket) close(socket)
#define SLEEP(t) usleep((int)1010.10f*(t));
pthread_mutexattr_t attr;
#endif

#define GET_IMG_EXE "./v4l2.exe"
#define IMAGE_FILE "v4l2.jpg"

#ifdef _WIN32
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
sendallow;

int  aim2exit = 0;
type_socket listen_socket;
static unsigned int g_threadNo_ = 0;

struct Network {
    unsigned char ip[INET_ADDRSTRLEN];
    unsigned int port;
    type_socket socket;
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
        unsigned char hgrp[24];
        unsigned char jgrp[24];
        unsigned char ngrp[24];
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

struct group_clazz {
    char name[24];
    char brief[24];
    char members[MAX_MENBERS_PER_GROUP][24];
    unsigned int usrCnt;
    unsigned char grpnm[24];
    char grpmrk[24];
    char grpbrf[24];
};

struct group_file {
    group_clazz group;
    group_file()
    {
        memset(this, 0, sizeof(*this));
    }
    unsigned int getSize() const { return sizeof(*this) + group.usrCnt * sizeof(member); }
}groups[MAX_GROUPS];


void pipesig_handler(int s) {
#ifdef _UNISTD_H
    write(STDERR_FILENO, "Signal: caught SIGPIPE!\n", 26);
#endif
}

int inst_mesg(int argc, char* argv[]);
template<typename T> int set_n_get_mem(T* shmem, int ndx = 0, int rw = 0);
void func_waitpid(int signo);
int queryNetworkParams(const char* username, Network& network, const int max = MAX_ACTIVE);
Network queryNetworkParams(int index);
int user_auth(char usr[24], char psw[24]);
int new_user(char usr[24], char psw[24]);
int user_is_line(char user[24]);
int set_user_line(char user[24], Network& netwk);
int set_user_peer(const char user[24], const char ip[INET_ADDRSTRLEN], const int port, type_socket sock);
int set_user_quit(char user[24]);
int get_user_seq(char user[24]);
int save_acnt();
int load_acnt();
int find_group(unsigned char basis[24]);
int host_group(char grpn[24], unsigned char brief[24]);
int join_group(int no, char usr[24], char psw[24]);
int exit_group(int no, char usr[24]);

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
    group_clazz group;
    int c, flg, num = 0;
    int qq = 0, logged = 0;
    int valrtn;
    char sd_bufs[256], rcv_txt[256];
    struct sockaddr_in sin;
#if !defined _WIN32
    struct ONLINE active[MAX_ACTIVE];
    memset(active, 0, sizeof(ONLINE) * MAX_ACTIVE);
#endif
    type_len len = static_cast<type_len>(sizeof(sin));
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
    fprintf(stdout, "accepted peer address [%s:%d] (@ %d/%d/%d %d:%d:%d)\n", IP, PORT, lt->tm_year + 1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
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
        if (getsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&val), &lol) == 0) {
            if (num == 3) {
                set_user_quit(user.usr);
                fprintf(stderr, "### socket status changed (%s): connection may going die.\n", strerror(errno));
#if defined _WIN32 || defined THREAD_PER_CONN || defined SOCK_CONN_TEST
                ;
#else
                closesocket(rcv_sock);
                exit(errno);
#endif
            }
            num++;
        }
        flg = recv(rcv_sock, rcv_txt, 256, 0);
        if (qq != flg && flg != 0)
        {
            g_threadNo_++;
            memcpy(&user, rcv_txt, sizeof(user));
            if (flg <= -1 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                if (num == 333) {
                    set_user_quit(user.usr);
                    fprintf(stderr, "----------------------------------------------------------------\
                    \n### client(%d) socket [%d] closed by itself just now.\n", g_threadNo_, rcv_sock);
                    closesocket(rcv_sock);
                }
                num++;
                continue;
            } else {
#ifdef _DEBUG
                fprintf(stdout, "----------------------------------------------------------------\
                \n>>> 1-RCV [%0x,%0x]: %s, %d, %d\n", user.uiCmdMsg, static_cast<unsigned>(*user.chk), user.usr, flg, g_threadNo_);
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
                    sprintf((sd_bufs + 8), "New user: %s", user.usr);
                    join_group(0, user.usr, (char*)("all"));
                    send(rcv_sock, sd_bufs, 48, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                } else if (valrtn == -2) {
                    strcpy((sd_bufs + 8), "Too many users.");
                    send(rcv_sock, sd_bufs, 28, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                } else if (valrtn >= 0) {
                    strcpy((sd_bufs + 8), "User already exists.");
                    send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                } else if (valrtn == -3) {
                    strcpy((sd_bufs + 8), "Same user name exist.");
                    send(rcv_sock, sd_bufs, 32, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                } else if (valrtn == -4) {
                    strcpy((sd_bufs + 8), "User name error.");
                    snres = send(rcv_sock, sd_bufs, 32, 0);
                    if (snres < 0) {
                        fprintf(stderr, "### socket status: %s\n", strerror(snres));
                    } else
                        fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                };
                break;
            } else if ((user.rsv == 0) && (user.uiCmdMsg == 0x1)) {
                // user.usr: 8; password: 32.
                sprintf(sd_bufs + 2, "%x", NEVAL(valrtn = user_auth(user.usr, user.psw)));
                if (valrtn == (logged = 1)) {
                    sprintf((sd_bufs + 8), "[%s] logging on successfully.", user.usr);
                    set_user_peer(user.usr, IP, PORT, rcv_sock);
                    Network sock;
                    memcpy(sock.ip, IP, INET_ADDRSTRLEN),
                        sock.port = PORT,
                        sock.socket = rcv_sock;
                    set_user_line(user.usr, sock);
                    memcpy(userName, user.usr, 24);
                    send(rcv_sock, sd_bufs, 64, 0);
                } else if (valrtn == 0) {
                    if (memcmp(user.usr, userName, 24) == 0) {
                        sprintf(sd_bufs + 2, "%x", NEVAL(-1));
                        sprintf((sd_bufs + 8), "Another [%s] is on line.", user.usr);
                        snres = send(rcv_sock, sd_bufs, 64, 0);
                        if (snres < 0) {
                            fprintf(stderr, "### socket status: %s\n", strerror(snres));
                        } else
                            fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                        continue;
                    } else {
                        sprintf((sd_bufs + 2), "%02x", 0xe8);
                        sprintf((sd_bufs + 8), "Logging status invalid, will close this socket.");
                        set_user_quit(user.usr);
                        send(rcv_sock, sd_bufs, 64, 0);
                        closesocket(rcv_sock);
                        fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                        continue;
                    }
                } else if (valrtn == -1) {
                    strcpy((sd_bufs + 8), "Error: check username/password again.");
                    send(rcv_sock, sd_bufs, 48, 0);
                    fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                    continue;
                } else {
                    fprintf(stdout, ">>> Unknown Error!\n");
                    continue;
                };
            } else if ((user.rsv == 0) && (user.uiCmdMsg == 0x3)) {
                strcpy((sd_bufs + 8), "Warning: user hasn't logged on yet.");
                send(rcv_sock, sd_bufs, 48, 0);
                fprintf(stdout, ">>> %s\n", sd_bufs + 8);
                continue;
            } else {
                strcpy((sd_bufs + 8), "Warning: please Register or Login at first.");
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
            while (logged) {
                memset(&user, 0, sizeof(user));
                flg = recv(rcv_sock, rcv_txt, 256, 0);
                if (flg < 0 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                    set_user_quit(user.usr);
                    fprintf(stderr, "### Lost connection with *[%s]: %s(%d)\n", userName, strerror(flg), flg);
                    logged = 0;
                    goto con_err1;
#if defined _WIN32 || defined THREAD_PER_CONN || defined SOCK_CONN_TEST
                    break;
#else
                    exit(errno);
#endif
                } else {
                    if (flg < 24) {
                        set_user_quit(user.usr);
                        if (flg == 0) {
                            fprintf(stderr, "### Socket disconnect normally.\n");
#if defined _WIN32 || defined THREAD_PER_CONN || defined SOCK_CONN_TEST
                            ;
#else
                            exit(errno);
#endif
                        } else
                            fprintf(stderr, "### Request param invalid.\n");
                        goto con_err0;
                    }
                    memcpy(&user, rcv_txt, flg);
                }
                if (flg > 0) {
#ifdef _DEBUG
                    fprintf(stdout, "----------------------------------------------------------------\
                    \n>>> 2-RCV [%x,%x]: %s, %d\n", user.uiCmdMsg, static_cast<unsigned>(*user.chk), user.usr, flg);
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
                        memcpy(sd_bufs + 8, susr, strlen(susr) + 1);
                        sndlen = 8 + strlen(susr) + 1;
                        break;
                    case 0x3:
                    {
                        flg = logged = 0;
                        set_user_quit(user.usr);
                        sprintf((sd_bufs + 8), "[%s] has logout.", user.usr);
                        send(rcv_sock, sd_bufs, 48, 0);
                        fprintf(stderr, "### [%0x, %x]: %s\n", sd_bufs[1], static_cast<unsigned>(*user.chk), sd_bufs + 8);
                        continue;
                    }
                    case 0x4:
                    {
                        valrtn = get_user_seq(user.usr);
                        sprintf((sd_bufs + 2), "%x", NEVAL(valrtn));
                        if (1 == user_auth(user.usr, user.psw) && user.npsw != nullptr)
                            strcpy(users[valrtn].psw, user.npsw);
                        else
                            memcpy((sd_bufs + 8), "Change password failure: user auth error.", 42);
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
                                if (sd_bufs + 8 * (c + 4) + 24 == '\0') {
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
                                const char* s = reinterpret_cast<char*>(&p2pnet.ip);
                                unsigned char t = 0;
                                while (1) {
                                    if (*s != '\0' && *s != '.') {
                                        t = t * 10 + *s - '0';
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
                                strcpy((sd_bufs + 32), "User was offline!");
                            }
                        } else {
                            valrtn = -3;
                            sprintf((sd_bufs + 32), "Get user network: No such user(%s)!", user.peer);
                        }
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        unsigned char *val = reinterpret_cast<unsigned char *>(&uiIP);
                        fprintf(stdout, ">>> get client peer [%u.%u.%u.%u:%s]\n", val[3], val[2], val[1], val[0], sd_bufs + 56);
                        sndlen = 64;
                    } break;
                    case 0x7:
                    {
                        strcpy((sd_bufs + 8), user.usr);
                        if (memcmp(user.chk, "NDT", 4) == 0) {
                            fprintf(stdout, "'%s' is communicating with '%s' via NDT.\n", user.usr, user.peer);
                        } else {
                            sprintf((sd_bufs + 32), "Check message error for Network Data Translation.");
                            sndlen = 64;
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
                                    sprintf((sd_bufs + 32), "Peer user ip or port value error.");
                                    sndlen = 72;
                                } else {
                                    char random = (rand() % 255 + '\1');
                                    char ndtmsg[32];
                                    memset(ndtmsg, 0, 32);
                                    memcpy(ndtmsg, &user, 4);
                                    sprintf(ndtmsg + 4, "%c", random);
                                    strcpy((char*)&user.status, "200");
                                    memcpy(ndtmsg + 8, &user.peer_mesg.msg, 24);
                                    if (-1 != send(ndtnet.socket, ndtmsg, 32, 0)) {
                                        sprintf((sd_bufs + 32), "[%c] NDT success to %s.", random, user.peer);
                                        sndlen = 80;
                                    } else {
                                        struct stat sock_stat;
                                        if (EBADF == fstat(ndtnet.socket, &sock_stat)) {
                                            fprintf(stdout, "Error: socket descriptor - %d.\n", ndtnet.socket);
                                        } else {
                                            fprintf(stdout, "Error: %s.\n", strerror(errno));
                                        }
                                        sprintf((sd_bufs + 32), "Got failure while sending a message.");
                                        sndlen = 72;
                                    }
                                }
                            } else {
                                valrtn = -2;
                                strcpy((sd_bufs + 32), "User was offline!");
                                sndlen = 56;
                            }
                        } else {
                            valrtn = -3;
                            sprintf((sd_bufs + 32), "Get user network: No such user(%s)!", user.peer);
                            sndlen = 92;
                        }
                    } break;
                    case 0x8:
                    {
#if !defined _WIN32
                        char *mesg = NULL;
                        if (vfork() == 0)
                        {
                            char *const agv[] = { (char*)"v4l2.exe", (char *)(0) };
                            valrtn = execvp(GET_IMG_EXE, agv);
                            mesg = strerror(errno);
                            sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                            strcpy((sd_bufs + 8), mesg);
                            sndlen = 8 + strlen(mesg) + 1;
                            fprintf(stdout, "Exec [ %s ] with execvp fail, %s.\n", GET_IMG_EXE, mesg);
                        } else {
                            wait(&valrtn);
                            if (errno != ECHILD) {
                                mesg = strerror(errno);
                            } else {
                                if (access(IMAGE_FILE, 0) == 0) {
                                    mesg = (char*)"sucess";
                                } else {
                                    mesg = (char*)"not save image file";
                                }
                            }
                            sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                            strcpy((sd_bufs + 8), mesg);
                            sndlen = 8 + strlen(mesg) + 1;
                            fprintf(stdout, "Make image with [ %s ], %s.\n", GET_IMG_EXE, mesg);
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
                        FILE * file = fopen(IMAGE_FILE, "rb");
                        if (file == NULL)
                        {
                            sprintf((sd_bufs + 8), "Fail open file \"%s\".", IMAGE_FILE);
                            fprintf(stdout, "%s\n", sd_bufs + 8);
                            sndlen = 8 + strlen(IMAGE_FILE) + 22;
                            break;
                        }
                        fseek(file, 0, SEEK_END);
                        long lSize = ftell(file);
                        rewind(file);
                        int num = lSize / sizeof(unsigned char);
                        if (num > 2097152)
                            num = 1024;
                        memset(sd_bufs + 8, num, 6);
                        unsigned char *pos = (unsigned char*)malloc(sizeof(unsigned char)*num);
                        if (pos == NULL)
                        {
                            sprintf((sd_bufs + 8), "Fail malloc for \"%s\".", IMAGE_FILE);
                            fprintf(stdout, "%s\n", sd_bufs + 8);
                            sndlen = 8 + strlen(IMAGE_FILE) + 24;
                            break;
                        }
                        int slice = 0;
                        while (int rcsz = fread(pos, sizeof(unsigned char), num, file) != 0 && !feof(file)) {
                            fprintf(stdout, "File \"%s\" size = %d, rcsz = %d, slice = %d.\n", IMAGE_FILE, num, rcsz, slice);
                            sprintf((sd_bufs + 14), "%04d", slice);
                            volatile int cur = 0;
                            for (volatile int i = 0; i <= num; i++) {
                                if ((i > 0) && (i % 224 == 0) || (i == num)) {
                                    sprintf((sd_bufs + 22), "%04d", i);
                                    send(rcv_sock, sd_bufs, 256, 0);
                                    memset(sd_bufs + 32, 0, 224);
                                    cur = 0;
                                }
                                memset(sd_bufs + 32 + cur, pos[i], 1);
                                SLEEP(1 / 10000);
                                cur++;
                            }
                            slice++;
                        }
                        free(pos);
                        fclose(file);
                    }
                    break;
                    case 0xA:
                    {
                        strcpy((sd_bufs + 8), "Groups list:\n");
                        for (c = 0; c < MAX_GROUPS; c++) {
                            if (strlen(groups[c].group.name) >> 0) {
                                sprintf(sd_bufs + 2, "%x", c);
                                strcpy((sd_bufs + 8 * (c + 4)), groups[c].group.name);
                            };
                            if (groups[c].group.name[0] == '\0')
                                break;
                        };
                        sndlen = 8 * (c + 4 + 3);
                    } break;
                    case 0xB:
                    {
                        valrtn = host_group(user.usr, user.hgrp);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -2) {
                            strcpy((sd_bufs + 8), "Host group rejected.");
                            sndlen = 32;
                        } else if (valrtn == -1) {
                            join_group(valrtn, user.usr, reinterpret_cast<char*>(user.jgrp));
                            sprintf((sd_bufs + 8), "%s, joined group: '%s'.", user.usr, user.jgrp);
                            sndlen = 72;
                        } else {
                            sprintf((sd_bufs + 8), "Created group: %s.", user.hgrp);
                            sndlen = 56;
                        }
                    } break;
                    case 0xC:
                    {
                        valrtn = join_group(find_group(user.jgrp), user.usr, reinterpret_cast<char*>(user.jgrp));
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -1) {
                            strcpy((sd_bufs + 8), "You have already in this group.");
                        } else if (valrtn == -2) {
                            strcpy((sd_bufs + 8), "Wrong pass code to this group.");
                        }
                        sndlen = 48;
                    } break;
                    case 0xD:
                    {
                        valrtn = exit_group(find_group(user.ngrp), user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == 0)
                            strcpy((sd_bufs + 8), "Leave group successfully.");
                        else
                            strcpy((sd_bufs + 8), "You aren't yet in this group.");
                        sndlen = 42;
                    } break;
                    case 0xE:
                    {
                        valrtn = find_group(user.ngrp);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -1) {
                            strcpy((sd_bufs + 8), "No such group.");
                            sndlen = 32;
                        } else {
                            strcpy((sd_bufs + 8), "User is member of group(s): \n");
                            for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
                                if (strlen(groups[valrtn].group.members[c]) >> 0) {
                                    strcpy((sd_bufs + 8 * (c + 4)), groups[valrtn].group.members[c]);
                                };
                            };
                            if (valrtn == -1 || groups[valrtn].group.members[c][0] == '\0')
                                break;
                            sndlen = 8 * (c + 4 + 4);
                        };
                    } break;
                    case 0xF:
                    { //loop1
                        valrtn = find_group(group.grpnm);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        for (int i = 0; i < MAX_ACTIVE; i++)
                            set_n_get_mem(&active[i], i);
                        if (valrtn == -1) {
                            if (-1 != user_is_line(user.usr)) {
                                sprintf(sd_bufs + 3, "%x", -1);
                                strcpy((sd_bufs + 8), user.usr);
                                strcpy((sd_bufs + 32), user.sign);
                                send(rcv_sock, sd_bufs, 48, 0);
                            };    //loop0
                        }//valrtn=-1
                        else {
                            for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
                                if (!strlen(groups[valrtn].group.members[c]) == 0)
                                {
                                    sprintf(sd_bufs + 3, "%x", -3);
                                    int uil = user_is_line(groups[valrtn].group.members[c]);
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
                    if (memcmp(user.chk, "P2P", 4) != 0)
                    {
                        send(rcv_sock, sd_bufs, sndlen, 0);
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
        };
        qq = flg;
        SLEEP(99);
    } while (!logged);

con_err0:
    for (int i = 0; i < MAX_ACTIVE; i++)
        set_n_get_mem(&active[i], i);
    memset(active[user_is_line(user.usr)].user, 0, 24);

con_err1:
    closesocket(rcv_sock);
    return NULL;
    };

type_thread_func commands(void *arg)
{
    char optionstr[24], name[24];
    do {
        scanf("%32s", reinterpret_cast<char*>(&optionstr));
        if (strcmp(optionstr, "quit") == 0) {
            closesocket(listen_socket);
            fprintf(stdout, "saving accounts data to file %s.\n", ACC_REC);
            save_acnt();
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
        strcpy(groups[0].group.name, "all");
        strcpy(groups[0].group.brief, "all");
        strcpy(groups[0].group.members[0], "iv9527");
    }
#ifdef _WIN32
    InitializeCriticalSection(&sendallow);
#else
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(sendallow), &attr);
#endif
    aim2exit = 0;
    pthread_t thread_ID;
#ifdef _WIN32
    SetConsoleTitle((
#ifdef _UNICODE
        LPCWSTR
#else
        LPCSTR
#endif
        )"chat server for network design");
    _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)commands, nullptr, 0, &thread_ID);
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
    local.sin_port = htons(servport);
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
        return -1;
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
        return -1;
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
        char IPdotdec[16]; IPdotdec;
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
        _beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, (void*)&test_socket, 0, &thread_ID);
#else
        _beginthreadex(nullptr, 0, (_beginthreadex_proc_type)monite, nullptr, 0, &thread_ID);
#endif
#else
#ifdef THREAD_PER_CONN
        pthread_create(&thread_ID, NULL, monite, NULL);
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
            memcpy(network.ip, active[c].netwk.ip, INET_ADDRSTRLEN);
            network.port = active[c].netwk.port;
            network.socket = active[c].netwk.socket;
            return 0;
        }
    }
    return -1;
}
// for Windows
Network queryNetworkParams(int index)
{
    Network network;
    memcpy(network.ip, users[index].netwk.ip, INET_ADDRSTRLEN);
    network.socket = users[index].netwk.socket;
    network.port = users[index].netwk.port;
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
    fwrite(groups, sizeof(group_clazz), MAX_GROUPS, dumpfile);
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
        fread(groups, sizeof(group_clazz), MAX_GROUPS, dumpfile);
        fclose(dumpfile);
        return 0;
    };
    flush_all();
};
int user_auth(char usr[24], char psw[24]) {
    char *n = usr, *p = psw;
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
    return -1;
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
            memcpy(active[i].netwk.ip, netwk.ip, INET_ADDRSTRLEN);
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
            memset(active[i].netwk.ip, 0, INET_ADDRSTRLEN);
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
            strcpy(reinterpret_cast<char*>(users[i].netwk.ip), ip);
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
    return -1;
};
int find_group(unsigned char basis[24]) {
    char *x = (char*)basis;
    if (x[0] == '\0')
        return -1;
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(groups[i].group.name, x) == 0)
            return i;
        for (int j = 0; j < MAX_MENBERS_PER_GROUP; j++) {
            if (strcmp(groups[i].group.members[j], x) == 0)
                return i;
        }
    };

    return -1;
};
int host_group(char grpn[24], unsigned char brief[24]) {
    char *name = grpn;
    unsigned char* brf = brief;
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(groups[i].group.name, name) == 0) {
            return -1;
        }
        if (strlen(groups[i].group.name) == 0) {
            strcpy(groups[i].group.name, name);
            strcpy(groups[i].group.brief, reinterpret_cast<char*>(brf));
            return i;
        };
    };
    return -2;
};
int join_group(int no, char usr[24], char brf[24]) {
    int i;
    char *m = usr, *b = brf;
    for (i = 0; i < MAX_MENBERS_PER_GROUP; i++) {
        if (strcmp(groups[no].group.members[i], m) == 0)
            return -1;
    };
    for (i = 0; i < MAX_GROUPS; i++) {
        if ((strlen(groups[no].group.members[i]) == 0)) {
            if (strcmp(groups[no].group.brief, b) == 0) {
                strcpy(groups[no].group.members[i], m);
                return i;
            } else {
                return -2;
            }
        };
    };
    return -3;
};
int exit_group(int no, char usr[24]) {
    char *m = usr;
    for (int i = 0; i < MAX_GROUPS; i++) {
        if ((strcmp(groups[no].group.members[i], m) == 0)) {
            strcpy(groups[no].group.members[i], "");
            if (i == 0)
                strcpy(groups[no].group.name, "");
            return 0;
        };
    };
    return -1;
};
