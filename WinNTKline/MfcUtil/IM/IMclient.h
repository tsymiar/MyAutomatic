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
typedef int SOCKET;
typedef pthread_mutex_t CRITICAL_SECTION;
typedef int                 BOOL;
typedef void*(*_beginthreadex_proc_type)(void*);
typedef pthread_t Pthreadt;
#define MAX_PATH          260
#define MB_OK                       0x00000000L
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define fprintf_s fprintf
#define scanf_s scanf
#define gets_s(c,v) gets(c)
#define TRUE true
inline unsigned int _beginthreadex(
    void* _Security,
    const pthread_attr_t * attr,
    _beginthreadex_proc_type start,
    void *__restrict arg,
    unsigned _InitFlag,
    Pthreadt * __newthread)
{
    return pthread_create(__newthread, attr, start, arg);
}
inline int closesocket(SOCKET socket) {
    return close(socket);
}
inline int Sleep(unsigned long t) {
    return usleep((int)1010.10f*(t));
}
inline char* strcpy_s(char(strDestination)[], char const* src) {
    return strcpy(strDestination, src);
}
inline int _countof(unsigned char* _Array) {
    return strlen((const char*)_Array) + 1;
}
inline char* _itoa(int val, char*str, int rdx) {
    sprintf(str, "%d", val);
    return str;
}
inline int InitializeCriticalSection(CRITICAL_SECTION* mutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_PROCESS_SHARED);
    return pthread_mutex_init(mutex, &attr);
}
inline int EnterCriticalSection(CRITICAL_SECTION* mutex) {
    return pthread_mutex_trylock(mutex);
}
inline int LeaveCriticalSection(CRITICAL_SECTION* mutex) {
    return pthread_mutex_unlock(mutex);
}
inline int DeleteCriticalSection(CRITICAL_SECTION* mutex) {
    return pthread_mutex_destroy(mutex);
}
inline int SetConsoleTitle(char* title) {
    return fprintf(stdout, "------%s------\n", title);
}
inline int MessageBox(int flag, char* message, char* title, int s) {
    return fprintf(stdout, "------%s------\n>>>\t%s\n", title, message);
}
inline int WSAGetLastError() {
    return errno;
}
inline int WSACleanup() { return 0; }
#endif

constexpr int DEFAULT_PORT = 8877;
constexpr char* filename = "recv.file";

struct P2P_NETWORK
{
    SOCKET socket = NULL;
    sockaddr_in addr;
};

typedef struct CLIENT
{
    SOCKET sock = NULL;
    sockaddr_in srvaddr;
    CRITICAL_SECTION wrcon;
    char url[64];
    int flag = 0;
    void* Dlg;
    void(*fp2p)(void*);
    int count = 0;
    struct LAST
    {
        char lastuser[24];
        char lastgrop[24];
    } last;
} st_client;

typedef struct IM_SETTING {
    int erno = -1;
    char addr[MAX_PATH] = { NULL };
    char IP[16];
    char auth[80];
    unsigned int PORT;
} st_setting;

typedef struct MSG_TRANS {
    unsigned char reserve;
    unsigned char uiCmdMsg;
    unsigned char retval[2];
    unsigned char type[4];
    union {
        unsigned char usr[24];
        unsigned char group_name[24];
    };
    union {
        char psw[24];
        char TOKEN[24];
        char peerIP[24];
        unsigned char group_mark[24];
    };
    union {
        unsigned char peer_name[24];
        unsigned char peer_port[24];
        unsigned char sign[24];
        unsigned char npsw[24];
        unsigned char group_host[24];
        unsigned char group_join[24];
    };
    struct PeerStruct
    {
        unsigned char rsv[2];
        unsigned char cmd[2];
        unsigned char val[4];
        unsigned char head[16];
    } peer_mesg;
    char more_mesg[40];
} st_trans;

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
    { 0x07,"聊天对象" },
    { 0x08,"拍照" },
    { 0x09,"下载照片" },
    { 0x0A,"用户信息" },
    { 0x0B,"创建群" },
    { 0x0C,"加入群" },
    { 0x0D,"查看群" },
    { 0x0E,"退群" },
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
    CHATWITH,
    V4L2IMG,
    GETTINGIMG,
    USERSIGN,
    HOSTGROUP,
    JOINGROUP,
    VIEWGROUP,
    EXITGROUP,
};
inline int checkPswValid(char* str)
{
    int z0 = 0;
    int zz = 0;
    int zZ = 0;
    int z_ = 0;
    for (int i = 0; i < (int)strlen(str); i++)
    {
        char ansi = str[i];
        if (ansi <= '9' && ansi >= '0')
        {
            z0 = 1;
        } else if (ansi <= 'z' && ansi >= 'a')
        {
            zz = 1;
        } else if (ansi <= 'Z' && ansi >= 'A')
        {
            zZ = 1;
        } else if (ansi > 127)
        {
            z_ = 0;
        } else
        {
            z_ = 1;
        }
    }
    return (z0 + zz + zZ + z_ == 4 ? 1 : 0);
}
int InitChat(st_setting* setting = NULL);
int StartChat(int erno,
    void
#ifndef _WIN32
    *
#endif
    (*func)(void*)
);
int SendChatMsg(st_trans* msg = NULL);
int GetStatus();
int callbackLog(char* usr, char* psw);
int CloseChat();
int p2pMessage(unsigned char *userName, int UserIP, unsigned int UserPort, char const *Message);
#ifdef _WIN32
int SetClientDlg(void* Wnd);
#endif
#endif
