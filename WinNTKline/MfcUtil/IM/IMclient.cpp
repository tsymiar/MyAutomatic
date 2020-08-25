#include "IMclient.h"

static st_sock socks = {};
static st_trans trans = {};
static st_client client = {};

// 参考该函数编写报文处理函数
#ifdef _WIN32
unsigned int __stdcall
#else
void*
#endif
runtime(void* param) {
    static char srv_net[32];
    static char rcv_buf[256];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    st_client* client = reinterpret_cast<st_client*>(param);
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(client->sock, rcv_buf, 256, 0);
        EnterCriticalSection(&wrcon);
        for (int c = 0; c < rcvlen; c++)
            fprintf(stdout, "%c", (unsigned char)rcv_buf[c]);
        fprintf(stdout, "\n");
        LeaveCriticalSection(&wrcon);
        if (rcvlen <= 0) {
            sprintf(srv_net, "Connection lost!\n%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
            char title[32];
            sprintf(title, "socket: %s", _itoa((int)client->sock, rcv_buf, 10));
            MessageBox(NULL, srv_net, title, MB_OK);
            if (rcvlen == -1) {
                client->flag = -1;
                closesocket(client->sock);
                exit(0);
            }
            '...';
            Sleep(100);
            continue;
        };
    };
};

int InitChat(st_sock* sock) {
    SetConsoleTitle("client v0.1");
    WSADATA wsaData;
    int erno = WSAStartup(0x202, &wsaData);
    if (erno == SOCKET_ERROR) {
        std::cerr << "WSAStartup failed with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(0);
    }
    InitializeCriticalSection(&client.wrcon);
    static char ipaddr[16];
    memset(ipaddr, 0, 16);
    if (sock == NULL || sock->IP[0] == '\0' || sock->IP[0] < 0) {
        fprintf_s(stdout, "Current OS is %d bit.\nNow enter server address: ", (int)(sizeof(void*) * 8));
        scanf_s("%s", &ipaddr, 16);
        if (*ipaddr != 0) {
            memcpy(socks.IP, &ipaddr, 16);
        }
    } else {
        memcpy(&ipaddr, sock->IP, 16);
        memcpy(&socks, sock, sizeof(st_sock));
    }
    if (sock != NULL && sock->PORT != 0) {
        socks.PORT = sock->PORT;
    } else {
        socks.PORT = DEFAULT_PORT;
    }
    client.srvaddr.sin_family = AF_INET;
#ifdef _UTILAPIS_
    inet_pton(AF_INET, ipaddr, (PVOID*)&client.srvaddr.sin_addr.s_addr);
#else
    client.srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
#endif
    client.srvaddr.sin_port = htons(socks.PORT);
    char title[32];
    sprintf(title, "client: %s", ipaddr);
    SetConsoleTitle(title);
    client.sock = socket(AF_INET, SOCK_STREAM, 0);
    BOOL bReuseaddr = TRUE;
    if (client.sock == INVALID_SOCKET) {
        std::cerr << "socket() invalid with error " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(0);
    }
    setsockopt(client.sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(BOOL));
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
    if (connect(client.sock, (struct sockaddr*)&client.srvaddr, sizeof(client.srvaddr)) == SOCKET_ERROR) {
        std::cerr << "connect() error " << "[" <<
            GetLastErrorToString(WSAGetLastError()).c_str()
            << "] " << std::endl;
        WSACleanup();
        exit(0);
    }
    Pthreadt threads;
    client.flag = 1;
    _beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &client, 0, &threads);
    return 0;
}

int StartChat(int erno,
    void
#ifndef _WIN32
    *
#endif
    (*func)(void*)
)
{
    client.erno = erno;
    if (erno != 0)
        return erno;
    else {
        if (func == NULL) {
            client.erno = -1;
            func = [](void*) {
#ifndef _WIN32
                return (void*)
#endif
                    fprintf_s(stdout, "Lambda null func.\n"); };
        }
    }
    Pthreadt threads;
    return (int)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Chat_Msg, (void*)func, 0, &threads);
}

int CloseChat()
{
    client.erno = -1;
    if (!closesocket(client.sock))
        DeleteCriticalSection(&client.wrcon);
    return WSACleanup();
}

int callbackLog(char* usr, char* psw)
{
    trans.uiCmdMsg = 0x1;
    memset(trans.username, 0, 24);
    memcpy(trans.username, usr, strlen(usr) + 1);
    memset(trans.password, 0, 24);
    memcpy(trans.password, psw, strlen(psw) + 1);
    send(client.sock, (char*)&trans, sizeof(st_trans), 0);
    return trans.uiCmdMsg;
}
#ifdef _WIN32
int SetClientDlg(void* Wnd)
{
    if (Wnd == NULL) {
        return -1;
    } else {
        client.Dlg = Wnd;
        return 0;
    }
};
#endif
int SendChatMesg(st_trans* msg)
{
    if (client.flag < 0) {
        MessageBox(NULL, "Connection status error, will exit!", "Quit", MB_OK);
        return (client.erno = -1);
    }
    int len = sizeof(trans);
    trans = { '\0', (unsigned char)(trans.uiCmdMsg & 0xff) };
    if (msg != NULL) {
        if ((int)msg->uiCmdMsg < 0)
            return -2;
        memcpy(&trans, msg, sizeof(st_trans));
    } else {
        send(client.sock, (char*)&trans, len, 0);
        return -1;
    }
    if (client.last.lastuser != NULL && client.last.lastgrop[0] != NULL)
    {
        memcpy(trans.username, client.last.lastuser, 24);
        memcpy(trans.group_name, client.last.lastgrop, 24);
    }
    return send(client.sock, (char*)&trans, len, 0);
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
    for (;;)
    {
        if (client.flag == 0 || P2Psock->socket == NULL)
            continue;
        char recvbuf[256];
        memset(recvbuf, 0, 256);
        int ret = recvfrom(P2Psock->socket, (char*)recvbuf, MAX_PACKET_SIZE, 0, (sockaddr*)&P2Psock->addr,
#ifdef _WIN32
            & len
#else
            (socklen_t*)(&len)
#endif
        );
        if (ret <= 0)
        {
            if (client.count < 3) {
                fprintf(stdout, "Recieve No Message: %s!\n", strerror(ret));
            } else {
                if (client.count > 30) {
                    fprintf(stdout, "Recieve error too many times!\n");
                    break;
                }
            }
            continue;
        } else {
            client.flag = 2;
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
            for (c = 0; c < ret; c++)
            {
                if (c > 0 && c % 32 == 0)
                    fprintf(stdout, "\n");
                fprintf(stdout, "%02x ", (unsigned char)recvbuf[c]);
            }
            fprintf(stdout, "\n");
            for (c = 0; c < ret; c++)
                fprintf(stdout, "%c", (unsigned char)recvbuf[c]);
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
    if (PrimaryUDP < 0)
    {
        std::cout << "UDP socket error!" << std::endl;
        return 0;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(socks.PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int bOptval = 1; // 端口复用
    int retSetsockopt = setsockopt(PrimaryUDP, SOL_SOCKET, SO_REUSEADDR, (char*)&bOptval, sizeof(bOptval));
    int MAXRETRY = 5;
    if (SOCKET_ERROR == retSetsockopt)
    {
        std::cout << "setsockopt() error!" << std::endl;
        return 0;
    }
    if (bind(PrimaryUDP, (const sockaddr*)&addr, (int)sizeof(addr)) < 0)
    {
        std::cout << "socket binding failure!" << std::endl;
        return 0;
    }
    if (client.flag != 2) {
        //没有接收到目标主机的回应，认为目标主机的端口
        //映射没有打开，那么发送请求到服务器要求“打洞”。
        st_trans MessageHost;
        memset(&MessageHost, 0, sizeof(st_trans));
        MessageHost.uiCmdMsg = PEER2P;
        memcpy(MessageHost.type, "P2P", 4);
        memcpy(MessageHost.username, trans.username, 24);
        memcpy(MessageHost.peer_name, userName, 24);
        //请求服务器“打洞”
        SendChatMesg(&MessageHost);
    }
    for (int trytime = 0; trytime < MAXRETRY; trytime++)
    {
        sockaddr_in remote;
        remote.sin_family = AF_INET;
        remote.sin_port = htons(UserPort);
#ifdef _WIN32
        remote.sin_addr.S_un.S_addr
#else
        remote.sin_addr.s_addr
#endif
            = htonl(UserIP);

        st_trans MessagePeer;
        memset(&MessagePeer, 0, sizeof(MessagePeer));
        MessagePeer.uiCmdMsg = PEER2P;
        memcpy(MessagePeer.username, userName, 24);
        //发送P2P消息头
        int ppres = sendto(PrimaryUDP, (const char*)&MessagePeer, 32, 0, (const sockaddr*)&remote, sizeof(remote));
        if (ppres < 0)
            return ppres;
        //发送P2P消息体
        ppres = sendto(PrimaryUDP, (const char*)&Message, (int)strlen(Message) + 1, 0, (const sockaddr*)&remote, sizeof(remote));
        if (ppres < 0)
            return ppres;
        memset(&MessagePeer, 0, sizeof(st_trans));
        memcpy(&MessagePeer, Message, sizeof(st_trans));
        if (client.count == 0) {
            //启动接收线程
            P2P_NETWORK pp_sock;
            pp_sock.addr = remote;
            pp_sock.socket = PrimaryUDP;
            Pthreadt threads;
            _beginthreadex(NULL, 0, RecvThreadProc, &pp_sock, 0, &threads);
        }
        client.count++;
        //等待接收消息线程修改标志
        for (int i = 0; i < 10; i++)
        {
            if (client.flag >= 0 && client.flag < 2)
                Sleep(300);
            else
                return -1;
        }
    }
    return 0;
}

int IsChatActive()
{
    return client.flag;
}

void SetChatActive(int flag)
{
    client.flag = flag;
}

int GetRecvState()
{
    return client.rcvndt;
}

void SetRecvState(int state)
{
    client.rcvndt = state;
}
