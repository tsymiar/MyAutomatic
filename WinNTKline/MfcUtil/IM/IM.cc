#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
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
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <signal.h>
#endif
#define NEVAL(a) (a) & 0xff
#define THREAD_NUM 20
#define DEFAULT_PORT 8877
#define MAX_USERS 99
#define MAX_ONLINE 30
#define MAX_MENBERS_PER_GROUP 11
#define MAX_GROUPS 10
#define ACC_REC "acnts"
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
#else
typedef int type_socket;
typedef socklen_t type_len;
typedef void *type_thread_func;
#define flush_all() fflush(stdin)
#define closesocket(socket) close(socket)
pthread_mutexattr_t attr;
#endif

#ifdef _WIN32
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
sendallow;

int  aimtoquit;
type_socket listen_socket;

struct user_clazz {
    char usr[24];
    char psw[24];
    struct peer {
        unsigned char ip[INET_ADDRSTRLEN];
        unsigned int port;
    } peer;
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

struct group_clazz {
    char name[24];
    char brief[24];
    char members[MAX_MENBERS_PER_GROUP][24];
    // unsigned int usrCnt;
    // member members[0];
};

struct group_file {
    group_clazz group;
    group_file()
    {
        memset(this, 0, sizeof(*this));
    }
    // unsigned int getSize() const { return sizeof(*this) + group.usrCnt * sizeof(member); }
}groups[MAX_GROUPS];

typedef struct user_socket {
    unsigned char rsv;
    unsigned char uiCmdMsg;
    unsigned char rtn[2];
    unsigned char chk[4];
    char usr[24];
    union {
        char psw[24];
        char peer[24];
        char TOKEN[24];
    };
    union {
        char sign[24];
        char npsw[24];
        unsigned char hgrp[24];
        unsigned char jgrp[24];
    };
} USER;

typedef struct group_socket {
    unsigned char rsv;
    unsigned char uiCmdMsg;
    unsigned char ret[2];
    unsigned char chk[4];
    char grpnm[24];
    char grpmrk[24];
    char grpbrf[24];
} st_GROUP;

struct LINE {
    type_socket sock_;
    char user[24];
} isline[MAX_ONLINE];

int _im_(int argc, char *argv[]);

void pipesig_handler(int s) {
    printf("Caught SIGPIPE!\n");
}

int main(
#ifndef _WIN32
    int argc, char* argv[]
#endif
)
{
#ifndef _WIN32
    if (fork() == 0)
    {
        signal(SIGPIPE, pipesig_handler);
#else
    int argc = 1;
    char* argv[] = { NULL };
#endif
    printf("IM chat server v0.%d for %dbit OS.\n", 1, sizeof(void*) * 8);
    _im_(argc, argv);
#ifndef _WIN32
    }
#endif
return 0;
}

type_socket g_sock;

int save_acnt();
int load_acnt();
int user_auth(char usr[24], char psw[24]);
int new_user(char usr[24], char psw[24]);
int user_is_line(char user[24]);
int set_user_line(char user[24], type_socket sock);
int set_user_quit(char user[24]);
int set_user_peer(const char user[24], const char ip[INET_ADDRSTRLEN], const int port);
int get_user_ndx(char user[24]);
int get_group_ndx(char group[24]);
int host_group(char grpn[24], unsigned char brief[24]);
int join_group(int no, char usr[24], char psw[24]);
int leave_group(int no, char usr[24]);

type_thread_func monite(void *arg)
{
    static USER user;
    st_GROUP group;
    int c, flg, num = 0;
    int qq = 0, logged = 0;
    int valrtn;
    char sd_bufs[256], rcv_txt[256];
    struct sockaddr_in sin;
    type_len len = (type_len)sizeof(sin);
    type_socket rcv_sock = 0;
    type_socket *sock = reinterpret_cast<type_socket*>(arg);
    char ipAddr[INET_ADDRSTRLEN];
#ifdef _WIN32
    if (-1 != (int)sock && (int)sock > 0) {
        if ((int)*sock != 0)
            rcv_sock = *sock;
        else
            return 0;
    } else 
#else
    rcv_sock = g_sock;
#endif // !_WIN32
    {
        rcv_sock = accept(listen_socket, (struct sockaddr*)&sin, &len);
    }
    struct sockaddr_in peerAddr;
    socklen_t peerLen = (socklen_t)sizeof(peerAddr);
    getpeername(rcv_sock, (struct sockaddr *)&peerAddr, &peerLen);
    const char* IP = inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr));
    const int PORT = ntohs(peerAddr.sin_port);
    printf("accepted peer address [%s:%d]\n", IP, PORT);
    bool set = true;
    setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&set, sizeof(bool));
    if (rcv_sock
#ifdef _WIN32
        == INVALID_SOCKET) {
        std::cerr << "accept() failed error " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        < 0) {
#endif
        std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
        return (type_thread_func)-1;
    };
    printf("socket monite: %d; waiting for massage.\n", rcv_sock);
    do {
        memset(sd_bufs, 0, 256);
        memset(rcv_txt, 0, 256);
        memset(&user, 0, sizeof(user));
        int val;
#ifdef _WIN32
        int
#else
        socklen_t
#endif
            lol = sizeof(int);
        if (getsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, &lol) == 0) {
            if (num == 3) {
                set_user_quit(user.usr);
                printf("### socket status changed (%s): connection may be die.\n", strerror(errno));
            }
            num++;
        }
        flg = recv(rcv_sock, rcv_txt, 256, 0);
        if (qq != flg && flg != 0)
        {
            memcpy(&user, rcv_txt, sizeof(user));
            if (flg <= -1 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                if (num == 333) {
                    set_user_quit(user.usr);
                    printf("----------------------------------------------------------------\
                    \n### client socket [%d] closed by itself just now.\n", rcv_sock);
                }
                continue;
            }
            else {
#ifdef _DEBUG
                printf("----------------------------------------------------------------\
                \n>>> 1-RCV [%0x,%0x]: %s, %d\n", user.uiCmdMsg, (unsigned)*user.chk, user.usr, flg);
                for (c = 0; c < flg; c++)
                {
                    if (c > 0 && c % 32 == 0)
                        printf("\n");
                    printf("%02x ", (unsigned char)rcv_txt[c]);
                }
                printf("\n");
#endif
            }
            int snres = -1;
            char userName[24]; 
            memcpy(sd_bufs, &user, 2);
            // rcv_txt: inc--8 bit crc_head, 24 bit username, 24 bit password.
            if ((user.rsv == 0) && (user.uiCmdMsg == 0)) {
                valrtn = new_user(user.usr, user.psw);
                sprintf(sd_bufs + 2, "%x", NEVAL(valrtn + 1));
                if (valrtn == -1) {
                    sprintf(sd_bufs + 8, "New user: %s", user.usr);
                    join_group(0, user.usr, (char*)"all");
                    send(rcv_sock, sd_bufs, 48, 0);
                    printf(">>> %s\n", sd_bufs + 8);
                    continue;
                }
                else if (valrtn == -2) {
                    strcpy((sd_bufs + 8), "Too many users.");
                    send(rcv_sock, sd_bufs, 28, 0);
                    printf(">>> %s\n", sd_bufs + 8);
                    continue;
                }
                else if (valrtn >= 0) {
                    strcpy((sd_bufs + 8), "User already exists.");
                    send(rcv_sock, sd_bufs, 32, 0);
                    printf(">>> %s\n", sd_bufs + 8);
                    continue;
                }
                else if (valrtn == -3) {
                    strcpy((sd_bufs + 8), "Same user name exist.");
                    send(rcv_sock, sd_bufs, 32, 0);
                    printf(">>> %s\n", sd_bufs + 8);
                    continue;
                }
                else if (valrtn == -4) {
                    strcpy((sd_bufs + 8), "User name error.");
                    snres = send(rcv_sock, sd_bufs, 32, 0);
                    if (snres < 0) {
                        printf("### socket status: %s\n", strerror(snres));
                    }
                    else
                        printf(">>> %s\n", sd_bufs + 8);
                    continue;
                };
                break;
            }
            else if ((user.rsv == 0) && (user.uiCmdMsg == 0x1)) {
                // user.usr: 8; password: 32.
                sprintf(sd_bufs + 2, "%x", NEVAL(valrtn = user_auth(user.usr, user.psw)));
                if (valrtn == (logged = 1)) {
                    sprintf(sd_bufs + 8, "[%s] logging on successfully.", user.usr);
                    set_user_peer(user.usr, IP, PORT);
                    set_user_line(user.usr, rcv_sock);
                    memcpy(userName, user.usr, 24);
                }
                else if (valrtn == 0) {
                    if (memcmp(user.usr, userName, 24) == 0)
                        sprintf(sd_bufs + 8, "[%s] already logged on.", user.usr);
                    else
                        sprintf(sd_bufs + 8, "logging status error, closing socket.\n");
                    snres = send(rcv_sock, sd_bufs, 64, 0);
                    if (snres < 0) {
                        printf("### socket status: %s\n", strerror(snres));
                    }
                    else
                        printf(">>> %s\n", sd_bufs + 8);
                    continue;
                }
                else if (valrtn == -1) {
                    strcpy((sd_bufs + 8), "Error: check username/password again.");
                    send(rcv_sock, sd_bufs, 48, 0);
                    printf(">>> %s\n", sd_bufs + 8);
                    continue;
                }
                else {
                    printf(">>> Unknown Error!\n");
                    continue;
                };
                send(rcv_sock, sd_bufs, 64, 0);
            }
            else {
                strcpy((sd_bufs + 8), "Warning: please Register or Login at first.");
                send(rcv_sock, sd_bufs, 56, 0);
                printf(">>> %s\n", sd_bufs + 8);
                continue;
            }
#ifdef _DEBUG
            printf(">>> 1-MSG [%0x,%0x]: ", sd_bufs[0], sd_bufs[1]);
            for (c = 2; c < 64; c++)
                printf("%c", (unsigned char)sd_bufs[c]);
            printf("\n");
#endif
            // set cur qq as last flg;
            qq = flg;
            while (logged) {
                memset(&user, 0, sizeof(user));
                flg = recv(rcv_sock, rcv_txt, 256, 0);
                if (flg < 0 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
                    set_user_quit(userName);
                    printf("### Lost connection with *[%s]: %s(%d)\n", userName, strerror(flg), flg);
                    logged = 0;
                    goto con_err1;
                    break;
                }
                else {
                    if (flg < 24) {
                        set_user_quit(user.usr);
                        if (flg == 0)
                            printf("### Socket disconnect normally.\n");
                        else
                            printf("### Request param invalid.\n");
                        goto con_err0;
                        break;
                    }
                    memcpy(&user, rcv_txt, flg);
                }
                if (flg > 0) {
#ifdef _DEBUG
                    printf("----------------------------------------------------------------\
                    \n>>> 2-RCV [%x,%x]: %s, %d\n", user.uiCmdMsg, (unsigned)*user.chk, user.usr, flg);
                    for (c = 0; c < flg; c++)
                    {
                        if (c > 0 && c % 32 == 0)
                            printf("\n");
                        printf("%02x ", (unsigned char)rcv_txt[c]);
                    }
                    printf("\n");
#endif
                }
                unsigned int buflen = 256;
                memset(sd_bufs, 0, buflen);
                if (user.rsv == 0)
                {
                    switch (sd_bufs[1] = user.uiCmdMsg)
                    {
                    case 0x01:
                        strcpy(sd_bufs + 8, "User already being on-line.");
                        buflen = 48;
                        break;
                    case 0x2:
                        memcpy(sd_bufs, &user, 8);
                        buflen = 8;
                        break;
                    case 0x3:
                    {
                        flg = logged = 0;
                        set_user_quit(user.usr);
                        sprintf(sd_bufs + 8, "[%s] has offline.", user.usr);
                        send(rcv_sock, sd_bufs, 48, 0);
                        printf("### [%0x, %x]: %s\n", sd_bufs[1], (unsigned)*user.chk, sd_bufs + 8);
                        continue;
                    }
                    case 0x4:
                    {
                        valrtn = get_user_ndx(user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (1 == user_auth(user.usr, user.psw) && user.npsw != '\0')
                            strcpy(users[valrtn].psw, user.npsw);
                        else
                            memcpy(sd_bufs + 8, "Change passwrod failure: user auth error.", 42);
                        buflen = 56;
                    } break;
                    case 0x5:
                    {
                        strcpy((sd_bufs + 8), "Users on-line list:\n");
                        for (c = 0; c < MAX_ONLINE; c++) {
                            if (strlen(isline[c].user) >> 0) {
                                sprintf(sd_bufs + 2, "%x", c);
                                strcpy((sd_bufs + 8 * (c + 4)), isline[c].user);
                            }
                            if (isline[c].user[0] == '\0')
                                break;
                        };
                        buflen = 8 * (c + 4 + 4);
                    } break;
                    case 0x6:
                    {
                        if (memcmp(user.chk, "P2P", 4) == 0) {
                            sprintf((sd_bufs + 8), "'%s' is communicating with '%s' by P2P.", user.usr, user.peer);
                            break;
                        }
                        unsigned int uiIP = 0;
                        valrtn = get_user_ndx(user.peer);
                        if (valrtn >= 0) {
                            if (user_is_line(user.peer) >= 0) {
                                printf("``` '%s' wants to P2P with '%s'\n", user.usr, user.peer);
                                const char* s = (char*)&users[valrtn].peer.ip;
                                unsigned char t = 0;
                                while (1) {
                                    if (*s != '\0' && *s != '.') {
                                        t = t * 10 + *s - '0';
                                    }
                                    else {
                                        uiIP = (uiIP << 8) + t;
                                        if (*s == '\0')
                                            break;
                                        t = 0;
                                    }
                                    s++;
                                };
                                sprintf((sd_bufs + 32), "%d", uiIP);
                                sprintf((sd_bufs + 54), "%d", users[valrtn].peer.port);
                                // p2p_req2usr
                            }
                            else {
                                valrtn = -2;
                                strcpy((sd_bufs + 32), "User was offline!");
                            }
                        }
                        else {
                            valrtn = -3;
                            strcpy((sd_bufs + 32), "Get user network: No such user!");
                        }
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        strcpy((sd_bufs + 8), user.peer);
                        unsigned char *val = (unsigned char *)&uiIP;
                        printf(">>> get client peer [%u.%u.%u.%u:%s]\n", val[3], val[2], val[1], val[0], sd_bufs + 54);
                        buflen = 64;
                    } break;
                    case 0x7:
                    {
                        memcpy(&group, &user, sizeof(group));
                        valrtn = get_group_ndx(group.grpnm);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -1) {
                            strcpy((sd_bufs + 8), "No such group.");
                            buflen = 32;
                        }
                        else {
                            strcpy((sd_bufs + 8), "User is member of group(s): \n");
                            for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
                                if (strlen(groups[valrtn].group.members[c]) >> 0) {
                                    strcpy((sd_bufs + 8 * (c + 4)), groups[valrtn].group.members[c]);
                                };
                            };
                            if (groups[valrtn].group.members[c][0] == '\0')
                                break;
                            buflen = 8 * (c + 4 + 4);
                        };
                    } break;
                    case 0x8:
                    {
                        valrtn = host_group(user.usr, user.jgrp);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -2) {
                            strcpy((sd_bufs + 8), "Host group rejected.");
                            buflen = 32;
                        }
                        else if (valrtn == -1) {
                            join_group(valrtn, user.usr, (char*)user.jgrp);
                            sprintf(sd_bufs + 8, "%s, joined group: '%s'.", user.usr, user.jgrp);
                            buflen = 72;
                        }
                        else {
                            sprintf(sd_bufs + 8, "Created group: %s.", user.hgrp);
                            buflen = 56;
                        }
                    } break;
                    case 0x9:
                    {
                        valrtn = join_group(get_group_ndx(user.usr), user.usr, (char*)user.jgrp);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -1) {
                            strcpy((sd_bufs + 8), "You have already in this group.");
                        }
                        else if (valrtn == -2) {
                            strcpy((sd_bufs + 8), "Wrong password to this group.");
                        }
                        buflen = 48;
                    } break;
                    case 0xA:
                    {
                        memcpy(&group, &user, sizeof(group));
                        valrtn = leave_group(get_group_ndx(group.grpnm), user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == 0)
                            strcpy((sd_bufs + 8), "Leave group successfully.");
                        else
                            strcpy((sd_bufs + 8), "You aren't yet in this group.");
                        buflen = 42;
                    } break;
                    case 0xB: // set user sign
                    {
                        valrtn = get_user_ndx(user.usr);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        strcpy(users[valrtn].intro, user.sign);
                        buflen = 8;
                    } break;
                    case 0xC:
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
                        buflen = 8 * (c + 4 + 3);
                    } break;
                    case 0xD:
                    { //loop1
                        valrtn = get_group_ndx(group.grpnm);
                        sprintf(sd_bufs + 2, "%x", NEVAL(valrtn));
                        if (valrtn == -1) {
                            if (-1 != user_is_line(user.usr)) {
                                sprintf(sd_bufs + 3, "%x", -1);
                                strcpy((sd_bufs + 8), user.usr);
                                strcpy((sd_bufs + 32), user.sign);
                                send(isline[valrtn].sock_, sd_bufs, 48, 0);
                            };    //loop0
                        }//valrtn=-1
                        else {
                            for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
                                if (!strlen(groups[valrtn].group.members[c]) == 0)
                                {
                                    sprintf(sd_bufs + 3, "%x", -3);
                                    int sss = user_is_line(groups[valrtn].group.members[c]);
                                    if (!(sss == -1)) {
                                        strcpy((sd_bufs + 8), user.usr);
                                        strcpy((sd_bufs + 32), user.sign);
                                        send(isline[sss].sock_, sd_bufs, 48, 0);
                                    }//if sss
                                }//if strlen
                            }//for
                        }//else
                    } break;
                    default: break;
                    }
                    send(rcv_sock, sd_bufs, buflen, 0);
#ifdef _DEBUG
                    printf(">>> 2-MSG [%0x,%0x]: ", sd_bufs[0], sd_bufs[1]);
                    for (c = 2; c < sizeof(user); c++)
                        printf("%c", (unsigned char)sd_bufs[c]);
                    printf("\n");
#endif
                }
#ifdef _WIN32
                Sleep(99);
#elif __linux
                usleep(99999);
#endif
            }
        };
        qq = flg;
#ifdef _WIN32
        Sleep(99);
#elif __linux
        usleep(99999);
#endif
    } while (!logged);

con_err0:
    memset(isline[user_is_line(user.usr)].user, 0, 24);

con_err1:
    closesocket(rcv_sock);

    return NULL;
    };

type_thread_func commands(void *arg)
{
    char optionstr[24], name[24];
    do {
        scanf("%s", (char*)&optionstr);
        if (strcmp(optionstr, "quit") == 0) {
            closesocket(listen_socket);
            printf("saving accounts data to file %s.\n", ACC_REC);
            save_acnt();
#ifdef _WIN32
            WSACleanup();
            DeleteCriticalSection(&sendallow);
#else
            pthread_mutex_destroy(&sendallow);
#endif
            aimtoquit = 1;
#ifdef _WIN32
            Sleep(2222);
#elif __linux
            sleep(2);
#endif
            exit(0);
        }
        if (strcmp(optionstr, "kick") == 0) {
            printf("Kick whom out?\n");
            scanf("%s", (char*)&name);
            int rtn = user_is_line(name);
            if (!(rtn == -1)) {
                closesocket(isline[rtn].sock_);
                printf("User %s kicked out!\n", name);
            };
        };
        if (strcmp(optionstr, "cls") == 0) {
            system("CLS");
        };
#ifdef _WIN32
        Sleep(99);
#elif __linux
        usleep(99999);
#endif
    } while (!(aimtoquit));
    return NULL;
};

int _im_(int argc, char * argv[]) {
    int servport = DEFAULT_PORT;
    if (argc == 2 && atoi(argv[1]) != 0) {
        servport = atoi(argv[1]);
    }
    if (!load_acnt()) {
        printf("accounts load finish from [%s].\n", ACC_REC);
    }
    else {
        strcpy(users[0].usr, "iv9527");
        strcpy(users[0].psw, "tesT123$");
        strcpy(groups[0].group.name, "all");
        strcpy(groups[0].group.brief, "all");
        strcpy(groups[0].group.members[0], "iv9527");
        strcpy(users[1].usr, "AAA");
        strcpy(users[1].psw, "AAA");
    }
#ifdef _WIN32
    InitializeCriticalSection(&sendallow);
#else
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(sendallow), &attr);
#endif
    aimtoquit = 0;
#ifdef _WIN32
    SetConsoleTitle((
#ifdef _UNICODE
        LPCWSTR
#else
        LPCSTR
#endif
        )"chat server for network design");
#endif
    pthread_t thread_ID;
#ifdef _WIN32
    _beginthreadex(NULL, 0, (_beginthreadex_proc_type)commands, NULL, 0, &thread_ID);
#else
    pthread_create(&thread_ID, NULL, commands, (void*)-1);
#endif
    int err = 0;
#ifdef _WIN32
    WSADATA wsaData;
    err = WSAStartup(0x202, &wsaData);
#endif
    if (err
#ifdef _WIN32
        == SOCKET_ERROR) {
        std::cerr << "WSAStartup failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        < 0) {
#endif
        std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
        return -1;
    }
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
    if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local))
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
    struct sockaddr_in listenAddr;
    socklen_t listenLen = (socklen_t)sizeof(listenAddr);
    getsockname(listen_socket, (struct sockaddr *)&listenAddr, &listenLen);
    printf("localhost listening [%s:%d].\n", inet_ntoa(listenAddr.sin_addr), servport);
    int thdcnt = 0;
    do {
#if (defined THREAD_PER_CONN ) || (defined TEST_SOCK)
        struct sockaddr_in from;
        type_len fromlen = (type_len)sizeof(from);
        char IPdotdec[16]; IPdotdec;
        type_socket test_socket = accept(listen_socket, (struct sockaddr*)&from, &fromlen);
#endif
#ifdef TEST_SOCK
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
        }
        else
        {
#ifdef _WIN32
            memcpy(IPdotdec, inet_ntoa(from.sin_addr), 16);
#else
            inet_ntop(AF_INET, (void *)&from.sin_addr, IPdotdec, 16);
#endif
            printf("accept [%s] success.\n", IPdotdec);
        }
#ifdef _DEBUG
        printf("socket test(%d): val=%d\n", c, test_socket);
#endif
        closesocket(test_socket);
#endif
        if (THREAD_NUM != 0 && thdcnt == THREAD_NUM)
            break;
#ifdef _WIN32
#ifdef THREAD_PER_CONN
        _beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, (void*)&test_socket, 0, &thread_ID);
#else
        _beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, NULL, 0, &thread_ID);
#endif
#else
#ifdef THREAD_PER_CONN
        g_sock = test_socket;
        pthread_create(&thread_ID, NULL, monite, (void*)&test_socket);
#else
        pthread_create(&thread_ID, NULL, monite, (void*)-1);
#endif
#endif
        thdcnt++;
#ifdef _WIN32
        Sleep(99);
#elif __linux
        usleep(99999);
#endif
        } while (!aimtoquit);
        printf("executing thread count = %d\n", thdcnt);
#ifdef _WIN32
        if (thdcnt == THREAD_NUM)
            while (1);
#endif
        return 0;
    }
//save accounts to file.
int save_acnt() {
    flush_all();
    FILE *dumpfile = NULL;
    dumpfile = fopen(ACC_REC, "w");
    if (dumpfile == NULL)
        return -1;
    fwrite(users, sizeof(USER), MAX_USERS, dumpfile);
    fwrite(groups, sizeof(st_GROUP), MAX_GROUPS, dumpfile);
    if (fclose(dumpfile) != 0)
        return -2;
    flush_all();
    return 0;
};
//load file system from disk.
int load_acnt() {
    FILE *dumpfile;
    flush_all();
    dumpfile = fopen(ACC_REC, "r");
    if (dumpfile == NULL)
        return -1;
    else {
        fread(users, sizeof(USER), MAX_USERS, dumpfile);
        fread(groups, sizeof(st_GROUP), MAX_GROUPS, dumpfile);
        fclose(dumpfile);
        return 0;
    };
    flush_all();
};
int user_auth(char usr[24], char psw[24]) {
    char *n = usr, *p = psw;
    for (int c = 0; c < MAX_USERS; c++) {
        if ((strcmp(n, users[c].usr) == 0) && (strcmp(p, users[c].psw) == 0)) {
            if (user_is_line(n) == -1) {
                return 1;   //success
            }
            else
                return 0;   //pass
        }
        else
            continue;
    };
    return -1; //wrong name or passwd
};
int new_user(char usr[24], char psw[24]) {
    if (usr[0] == '\0')
        return -4;
    if (0 <= get_user_ndx(usr)) {
        return -3;
    }
    int c;
    char *n = usr;
    char *p = psw;
    for (c = 0; c < MAX_USERS; c++) {
        if (strcmp(n, users[c].usr) == 0)
            return c;
    };
    for (c = 0; c < MAX_USERS; c++) {
        if (users[c].usr[0] == '\0') {
            strcpy(users[c].usr, n);
            strcpy(users[c].psw, p);
            strcpy(users[c].intro, "intro weren't set.\n");
            return -1;
        };
    };
    return -2;
};
int user_is_line(char user[24]) {
    int c;
    char *tomatch = user;
    for (c = 0; c < MAX_ONLINE; c++) {
        if (strcmp(tomatch, isline[c].user) == 0)
            return c;
    }
    return -1;
};
int set_user_line(char user[24], type_socket sock) {
    for (int i = 0; i < MAX_ONLINE; i++) {
        if (isline[i].user[0] == '\0')
        {
            memcpy(isline[i].user, user, 24);
            isline[i].sock_ = sock;
            break;
        }
    }
    return 0;
};
int set_user_quit(char user[24]) {
    char *n = user;
    for (int i = 0; i < MAX_ONLINE; i++) {
        if (strcmp(n, isline[i].user) == 0)
        {
            memset(isline[i].user, 0, 24);
            isline[i].sock_ = 0;
            break;
        }
    }
    return 0;
}
int set_user_peer(const char user[24], const char ip[INET_ADDRSTRLEN], const int port)
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
    for (int c = 0; c < MAX_USERS; c++) {
        if (strcmp(u, users[c].usr) == 0) {
            strcpy((char*)users[c].peer.ip, ip);
            users[c].peer.port = port;
            return 0;
        }
    }
    return -4;
}
int get_user_ndx(char user[24]) {
    int c;
    char *n = user;
    for (c = 0; c < MAX_USERS; c++) {
        if (strcmp(n, users[c].usr) == 0)
            return c;
    };
    return -1;
};
int get_group_ndx(char group[24]) {
    int c;
    char *x = group;
    for (c = 0; c < MAX_GROUPS; c++) {
        if (strcmp(groups[c].group.name, x) == 0)
            return c;
    };
    return -1;
};
int host_group(char grpn[24], unsigned char brief[24]) {
    int c;
    char *name = grpn;
    unsigned char* brf = brief;
    for (c = 0; c < MAX_GROUPS; c++) {
        if (strcmp(groups[c].group.name, name) == 0) {
            return -1;
        }
        if (strlen(groups[c].group.name) == 0) {
            strcpy(groups[c].group.name, name);
            strcpy(groups[c].group.brief, (char*)brf);
            return c;
        };
    };
    return -2;
};
int join_group(int no, char usr[24], char brf[24]) {
    int c;
    char *m = usr, *b = brf;
    for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
        if (strcmp(groups[no].group.members[c], m) == 0)
            return -1;
    };
    for (c = 0; c < MAX_GROUPS; c++) {
        if ((strlen(groups[no].group.members[c]) == 0)) {
            if (strcmp(groups[no].group.brief, b) == 0) {
                strcpy(groups[no].group.members[c], m);
                return c;
            }
            else
                return -2;
        };
    };
    return -3;
};
int leave_group(int no, char usr[24]) {
    int c;
    char *m = usr;
    for (c = 0; c < MAX_GROUPS; c++) {
        if ((strcmp(groups[no].group.members[c], m) == 0)) {
            strcpy(groups[no].group.members[c], "");
            if (c == 0)
                strcpy(groups[no].group.name, "");
            return 0;
        };
    };
    return -1;
};
