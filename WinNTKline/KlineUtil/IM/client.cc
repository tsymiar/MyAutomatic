#include "client.h"

static StSock g_socks = {};
static StClient g_client = {};
static StMsgContent g_content = {};

// 参考该函数编写报文处理函数
#ifdef _WIN32
unsigned int __stdcall
#else
void*
#endif
runtime(void* param)
{
    static char srv_net[32];
    static char rcv_buf[256];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    StClient* g_client = reinterpret_cast<StClient*>(param);
    while (1) {
        if (g_client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(g_client->sock, rcv_buf, 256, 0);
        EnterCriticalSection(&wrcon);
        for (int c = 0; c < rcvlen; c++)
            fprintf(stdout, "%c", (unsigned char)rcv_buf[c]);
        fprintf(stdout, "\n");
        LeaveCriticalSection(&wrcon);
        if (rcvlen <= 0) {
            snprintf(srv_net, 32, "Connection lost!\n%s:%d", inet_ntoa(g_client->srvaddr.sin_addr), g_client->srvaddr.sin_port);
            char title[32];
            snprintf(title, 32, "socket: %s", _itoa((int)g_client->sock, rcv_buf, 10));
            MessageBox(0, srv_net, title, MB_OK);
            if (rcvlen == -1) {
                g_client->flag = -1;
                closesocket(g_client->sock);
                exit(0);
            }
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
            // "...";
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
            Sleep(100);
            continue;
        };
    };
};

int InitChat(StSock* sock)
{
    SetConsoleTitle("IM client v0.1");
    WSADATA wsaData;
    if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR) {
        std::cerr << "WSAStartup failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(0);
    }
    InitializeCriticalSection(&g_client.wrcon);
    static char ipaddr[16];
    memset(ipaddr, 0, 16);
    if (sock == NULL || sock->IP[0] == '\0' || sock->IP[0] < 0) {
        fprintf_s(stdout, "Current OS is %d bit.\nEnter server IP: ", (int)(sizeof(void*) * 8));
        scanf_s("%15s", (char*)&ipaddr, 16);
        if (*ipaddr != 0) {
            memcpy(g_socks.IP, &ipaddr, 16);
        }
    } else {
        memcpy(&ipaddr, sock->IP, 16);
        memcpy(&g_socks, sock, sizeof(StSock));
    }
    if (sock != NULL && sock->PORT != 0) {
        g_socks.PORT = sock->PORT;
    } else {
        g_socks.PORT = DEFAULT_PORT;
    }
    g_client.srvaddr.sin_family = AF_INET;
#ifdef _UTILAPIS_
    inet_pton(AF_INET, ipaddr, (PVOID*)&g_client.srvaddr.sin_addr.s_addr);
#else
    g_client.srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
#endif
    g_client.srvaddr.sin_port = htons(g_socks.PORT);
    char title[32];
    snprintf(title, 27, "IM client: %s", ipaddr);
    SetConsoleTitle(title);
    g_client.sock = socket(AF_INET, SOCK_STREAM, 0);
    BOOL bReuseaddr = TRUE;
    if (g_client.sock == INVALID_SOCKET) {
        std::cerr << "socket() invalid with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(0);
    }
    setsockopt(g_client.sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(BOOL));
    SetRecvState(RCV_TCP);
    return 0;
}

std::string GetLastErrorToString(int errorCode)
{
#ifdef _WIN32
    char* text;
    // 设置FORMAT_MESSAGE_ALLOCATE_BUFFER标志分配内存时需要LocalFree释放
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&text, 0, NULL);
    std::string result(text);
    LocalFree(text);
    return result;
#else
    return strerror(errorCode);
#endif
}
#ifdef _WIN32
unsigned int
__stdcall
Chat_Msg(void(*func)(void*))
#else
void* Chat_Msg(void* func)
#endif
{
    if (connect(g_client.sock, (struct sockaddr*)&g_client.srvaddr, sizeof(g_client.srvaddr)) == SOCKET_ERROR) {
        std::cerr << "call connect() fail " << "[" <<
            GetLastErrorToString(WSAGetLastError()).c_str()
            << "] " << std::endl;
        WSACleanup();
        exit(0);
    }
    Pthreadt threads;
    g_client.flag = 1;
    _beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &g_client, 0, &threads);
    return 0;
}

int StartChat(int error,
    void
#ifndef _WIN32
    *
#endif
    (*func)(void*)
)
{
    g_client.error = error;
    if (error != 0)
        return error;
    else {
        if (func == NULL) {
            g_client.error = -1;
            func = [](void*) {
                fprintf_s(stdout, "Lambda null func.\n");
#ifndef _WIN32
                return (void*)0;
#endif
            };
        }
    }
    Pthreadt threads;
    return (int)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Chat_Msg, (void*)func, 0, &threads);
}

int CloseChat()
{
    g_client.error = -1;
    if (!closesocket(g_client.sock))
        DeleteCriticalSection(&g_client.wrcon);
    return WSACleanup();
}

int callbackLog(char* usr, char* psw)
{
    g_content.uiCmdMsg = 0x1;
    memcpy(g_content.username, usr, 24);
    memcpy(g_content.password, psw, 24);
    send(g_client.sock, (char*)&g_content, sizeof(StMsgContent), 0);
    return g_content.uiCmdMsg;
}
#ifdef _WIN32
int SetClientDlg(void* Wnd)
{
    if (Wnd == NULL) {
        return -1;
    } else {
        g_client.Dlg = Wnd;
        return 0;
    }
};
#endif
int SendClientMessage(StMsgContent* msg)
{
    if (g_client.flag < 0) {
        MessageBox(0, const_cast<char*>("Connection status error, will exit!"), const_cast<char*>("Quit"), MB_OK);
        return (g_client.error = -1);
    }
    int len = sizeof(g_content);
    g_content = { '\0', (unsigned char)(g_content.uiCmdMsg & 0xff) };
    if (msg != NULL) {
        memcpy(&g_content, msg, sizeof(StMsgContent));
    } else {
        send(g_client.sock, (char*)&g_content, len, 0);
        return -1;
    }
    if (g_client.last.lastuser[0] != 0 && g_client.last.lastgrop[0] != 0) {
        memcpy(g_content.username, g_client.last.lastuser, 24);
        memcpy(g_content.group_name, g_client.last.lastgrop, 24);
    }
    return send(g_client.sock, (char*)&g_content, len, 0);
}

/*P2P接收消息线程*/
#ifdef _WIN32
unsigned int __stdcall
#else
void*
#endif
RecvThreadProc(void* PrimaryUDP)
{
    int MAX_PACKET_SIZE = 256;
    P2P_NETWORK* P2Psock = reinterpret_cast<P2P_NETWORK*>(PrimaryUDP);
    int len = sizeof(P2Psock->addr);
    for (;;) {
        if (g_client.flag == 0 || P2Psock->socket == 0)
            continue;
        char rcvbuf[256];
        memset(rcvbuf, 0, 256);
        int ret = recvfrom(P2Psock->socket, reinterpret_cast<char*>(rcvbuf), MAX_PACKET_SIZE, 0, (sockaddr*)&P2Psock->addr,
#ifdef _WIN32
            & len
#else
            (socklen_t*)(&len)
#endif
        );
        if (ret <= 0) {
            if (g_client.count < 3) {
                fprintf(stdout, "Receive No Message: %s!\n", strerror(ret));
            } else {
                if (g_client.count > 30) {
                    fprintf(stdout, "Receive failed too many times!\n");
                    break;
                }
            }
            continue;
        } else {
            g_client.flag = 2;
            in_addr peer;
#ifdef _WIN32
            peer.S_un.S_addr = P2Psock->addr.sin_addr.S_un.S_addr;
#else
            peer.s_addr = P2Psock->addr.sin_addr.s_addr;
#endif
            int port = P2Psock->addr.sin_port;
            fprintf(stdout, "Message from [%s:%d] >>\n", inet_ntoa(peer), port);
            fprintf(stdout, "----------------------------------------------------------------\n");
            int c = 0;
            for (c = 0; c < ret; c++) {
                if (c > 0 && c % 32 == 0)
                    fprintf(stdout, "\n");
                fprintf(stdout, "%02x ", (unsigned char)rcvbuf[c]);
            }
            fprintf(stdout, "\n");
            for (c = 0; c < ret; c++)
                fprintf(stdout, "%c", (unsigned char)rcvbuf[c]);
            fprintf(stdout, "\n");
        }
    }
    return 0;
}
/*P2P主体函数*/
//流程：首先，直接向某个客户的外网IP发送消息，如果会话状态无效，则该消息发送端等待超时；
//判断超时后，发送端发送请求到服务器要求“打洞”，然后服务器请求该客户向本机发送打洞消息。
//重复MAXRETRY次
int p2pMessage(unsigned char* userName, int UserIP, unsigned int UserPort, char const* Message)
{
    SOCKET PrimaryUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (PrimaryUDP < 0) {
        std::cout << "UDP socket error!" << std::endl;
        return 0;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_socks.PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int bOptval = 1; // 端口复用
    int retSetsockopt = setsockopt(PrimaryUDP, SOL_SOCKET, SO_REUSEADDR, (char*)&bOptval, sizeof(bOptval));
    int MAXRETRY = 5;
    if (SOCKET_ERROR == retSetsockopt) {
        std::cout << "setsockopt() error!" << std::endl;
        return 0;
    }
    if (bind(PrimaryUDP, (const sockaddr*)&addr, (int)sizeof(addr)) < 0) {
        std::cout << "socket binding failure!" << std::endl;
        return 0;
    }
    if (g_client.flag != 2) {
        //没有接收到目标主机的回应，认为目标主机的端口
        //映射没有打开，那么发送请求到服务器要求“打洞”。
        StMsgContent MessageHost;
        memset(&MessageHost, 0, sizeof(StMsgContent));
        MessageHost.uiCmdMsg = PEER2P;
        memcpy(MessageHost.type, "P2P", 4);
        memcpy(MessageHost.username, g_content.username, 24);
        memcpy(MessageHost.peer_name, userName, 24);
        //请求服务器“打洞”
        SendClientMessage(&MessageHost);
    }
    for (int trytime = 0; trytime < MAXRETRY; trytime++) {
        sockaddr_in remote;
        remote.sin_family = AF_INET;
        remote.sin_port = htons(UserPort);
#ifdef _WIN32
        remote.sin_addr.S_un.S_addr
#else
        remote.sin_addr.s_addr
#endif
            = htonl(UserIP);

        StMsgContent MessagePeer;
        memset(&MessagePeer, 0, sizeof(MessagePeer));
        MessagePeer.uiCmdMsg = PEER2P;
        memcpy(MessagePeer.username, userName, 24);
        //发送P2P消息头
        int ppres = sendto(PrimaryUDP, (const char*)&MessagePeer, 32, 0, (const sockaddr*)&remote, sizeof(remote));
        if (ppres < 0)
            return ppres;
        //发送P2P消息体
        ppres = sendto(PrimaryUDP, (const char*)&Message, sizeof(StMsgContent), 0, (const sockaddr*)&remote, sizeof(remote));
        if (ppres < 0)
            return ppres;
        memset(&MessagePeer, 0, sizeof(StMsgContent));
        memcpy(&MessagePeer, Message, sizeof(StMsgContent));
        if (g_client.count == 0) {
            //启动接收线程
            P2P_NETWORK pp_sock;
            pp_sock.addr = remote;
            pp_sock.socket = PrimaryUDP;
            Pthreadt threads;
            _beginthreadex(NULL, 0, RecvThreadProc, &pp_sock, 0, &threads);
        }
        g_client.count++;
        //等待接收消息线程修改标志
        for (int i = 0; i < 10; i++) {
            if (g_client.flag >= 0 && g_client.flag < 2)
                Sleep(300);
            else
                return -1;
        }
    }
    return 0;
}

int IsChatActive()
{
    return g_client.flag;
}

void SetChatActive(int flag)
{
    g_client.flag = flag;
}

int GetRecvState()
{
    return g_client.rcvstat;
}

void SetRecvState(int state)
{
    g_client.rcvstat = state;
}
