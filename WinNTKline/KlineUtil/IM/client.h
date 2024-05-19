#ifndef _IMCIENT_H
#define _IMCIENT_H
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <map>
#ifdef _WIN32
#pragma comment(lib, "WS2_32.lib")
#pragma warning (disable:4477)
#pragma warning (disable:4819)
#ifdef _UTILAPIS_
#include <WS2tcpip.h>
#else
#include <winsock2.h>
#include <Ws2ipdef.h>
#endif
#include <process.h>
#include <conio.h>
typedef unsigned int Pthreadt;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <mutex>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>
#include <signal.h>
#include <chrono>
#include <thread>
typedef int SOCKET;
typedef pthread_mutex_t CRITICAL_SECTION;
typedef int                 BOOL;
typedef void* (*_beginthreadex_proc_type)(void*);
typedef pthread_t Pthreadt;
typedef struct WSADATA {
    char w;
} WsaData;
#define MAX_PATH          260
#define MB_OK             0x00000000L
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define fprintf_s fprintf
#define gets_s(c,v) fgets(c,v,stdin)
#define TRUE true
#ifndef scanf_s
#define scanf_s(x, y, ...) scanf((x), (y))
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif
#endif
inline unsigned int _beginthreadex(
    void* _Security,
    const pthread_attr_t* attr,
    _beginthreadex_proc_type start,
    void* __restrict arg,
    unsigned _InitFlag,
    Pthreadt* __newthread)
{
    return pthread_create(__newthread, attr, start, arg);
}
inline int closesocket(SOCKET socket)
{
    return close(socket);
}
inline void Sleep(unsigned long ms)
{
    std::this_thread::sleep_for(std::chrono::microseconds(ms));
}
inline char* strcpy_s(char(strDestination)[], char const* src)
{
    return strcpy(strDestination, src);
}
inline int _countof(unsigned char* _Array)
{
    return strnlen((const char*)_Array, 256) + 1;
}
inline char* _itoa(int val, char* str, int rdx)
{
    if (rdx == 16)
        snprintf(str, 16, "%x", val);
    else if (rdx == 10)
        snprintf(str, 16, "%d", val);
    return str;
}
inline int InitializeCriticalSection(CRITICAL_SECTION* mutex)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_PROCESS_SHARED);
    return pthread_mutex_init(mutex, &attr);
}
inline int EnterCriticalSection(CRITICAL_SECTION* mutex)
{
    return pthread_mutex_trylock(mutex);
}
inline int LeaveCriticalSection(CRITICAL_SECTION* mutex)
{
    return pthread_mutex_unlock(mutex);
}
inline int DeleteCriticalSection(CRITICAL_SECTION* mutex)
{
    return pthread_mutex_destroy(mutex);
}
inline int SetConsoleTitle(const char* title)
{
    return fprintf(stdout, "------ %s ------\n", title);
}
inline int MessageBox(int flag, char* message, char* title, int s)
{
    return fprintf(stdout, "------ %s ------\n>>>\t%s\n", title, message);
}
inline int WSAGetLastError()
{
    return errno;
}
inline int WSAStartup(int, WSADATA*) { return 0; }
inline int WSACleanup() { return 0; }
#endif

constexpr int DEFAULT_PORT = 8877;
constexpr const char* filename = "NoNameFile";

struct P2P_NETWORK {
    SOCKET socket = 0;
    sockaddr_in addr;
};

typedef struct CLIENT {
    SOCKET sock = 0;
    sockaddr_in srvaddr;
    CRITICAL_SECTION wrcon;
    char url[64];
    int flag = 0;
    volatile int rcvstat = 0;
    void* Dlg;
    void(*fp2p)(void*);
    int count = 0;
    int error = -1;
    struct LAST {
        char lastuser[24];
        char lastgrop[24];
    } last;
} StClient;

typedef struct IM_SOCK {
    char addr[MAX_PATH] = { '\0' };
    char form[0x50];
    char IP[INET_ADDRSTRLEN];
    unsigned int PORT;
} StSock;

struct MainMessage {
    unsigned char message[16];
    unsigned char status[8];
};

struct ExtraMessage {
    unsigned char rsv[2];
    unsigned char cmd[2];
    unsigned char val[4];
    struct MainMessage msg;
};

typedef struct MSG_CONTENT {
    unsigned char reserve;
    unsigned char uiCmdMsg;
    unsigned char value[2];
    unsigned char type[4];
    union {
        unsigned char username[24];
        struct MainMessage rcv_msg;
    };
    union {
        char password[24];
        char TOKEN[24];
        char peerIP[24];
        char ndt_msg[24];
        unsigned char group_mark[24];
    };
    union {
        unsigned char user_sign[24];
        unsigned char user_newpass[24];
        unsigned char peer_name[24];
        unsigned char peer_port[24];
        unsigned char group_name[24];
        unsigned char group_host[24];
        unsigned char group_join[24];
    };
    struct ExtraMessage ext_msg;
} StMsgContent;

struct MENU {
    int key;
    std::string value;
};

const MENU menus[] =
{
    { 0x00,"命令菜单" },
    { 0x01,"注册" },
    { 0x02,"登陆" },
    { 0x04,"帮助" },
    { 0x03,"登出" },
    { 0x05,"设置密码" },
    { 0x06,"在线用户列表" },
    { 0x08,"P2P" },
    { 0x07,"NDT" },
    { 0x09,"IMG" },
    { 0x0A,"下载" },
    { 0x0B,"群列表" },
    { 0x0C,"群成员" },
    { 0x0D,"建群" },
    { 0x0E,"进群" },
    { 0x0F,"退群" },
};

enum  EM_MENU {
    CHAT = -2,
    TALK,
    REGISTER = 0,
    LOGIN,
    IUSER,
    LOGOUT,
    SETPSW,
    ONLINE,
    PEER2P,
    NETNDT,
    V4L2IMG,
    GETIMAGE,
    VIEWGROUP,
    USERGROUP,
    HOSTGROUP,
    JOINGROUP,
    EXITGROUP,
};

enum  RCV_STATE {
    RCV_ERR = -1,
    RCV_SCC,
    RCV_TCP,
    RCV_P2P,
    RCV_NDT
};

inline int checkPswValid(char* str)
{
    int z0 = 0;
    int zz = 0;
    int zZ = 0;
    int z_ = 0;
    for (int i = 0; i < (int)strnlen(str, 256); i++) {
        char ansi = str[i];
        if (ansi <= '9' && ansi >= '0') {
            z0 = 1;
        } else if (ansi <= 'z' && ansi >= 'a') {
            zz = 1;
        } else if (ansi <= 'Z' && ansi >= 'A') {
            zZ = 1;
        } else if (ansi > 127) {
            z_ = 0;
        } else {
            z_ = 1;
        }
    }
    return (z0 + zz + zZ + z_ == 4 ? 1 : 0);
}
int InitChat(StSock* sock = NULL);
int StartChat(int error,
    void
#ifndef _WIN32
    *
#endif
    (*func)(void*)
);
int SendClientMessage(StMsgContent* msg = NULL);
int callbackLog(char* usr, char* psw);
int CloseChat();
int p2pMessage(unsigned char* userName, int UserIP, unsigned int UserPort, char const* Message);
#ifdef _WIN32
int SetClientDlg(void* Wnd);
#endif
int IsChatActive();
void SetChatActive(int flag);
int GetRecvState();
void SetRecvState(int state);
#endif
