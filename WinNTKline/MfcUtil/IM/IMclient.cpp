#include "IMclient.h"

using namespace std;

st_trans trans;
st_client client;
st_setting setting;

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
                break;
            }
            continue;
            Sleep(100);
            '...';
        };
    };
};

int InitChat(st_setting* sets) {
    SetConsoleTitle("client v0.1");
    WSADATA wsaData;
    int erno = WSAStartup(0x202, &wsaData);
    if (erno == SOCKET_ERROR) {
        cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        exit(0);
    }
    InitializeCriticalSection(&client.wrcon);
    static char ipaddr[16];
    memset(ipaddr, 0, 16);
    if (sets == NULL || sets->IP[0] == '\0' || sets->IP[0] < 0) {
        fprintf_s(stdout, "Current OS is %d bit.\nNow enter server address: ", sizeof(void*) * 8);
        scanf_s("%16s", &ipaddr, 16);
        if (*ipaddr != 0) {
            memcpy(setting.IP, &ipaddr, 16);
        }
        if (sets != NULL && sets->PORT != 0) {
            setting.PORT = sets->PORT;
        }
    } else {
        memcpy(&ipaddr, sets->IP, 16);
        memcpy(&setting, sets, sizeof(st_setting));
    }
    client.srvaddr.sin_family = AF_INET;
#ifdef _UTILAPIS_
    inet_pton(AF_INET, ipaddr, (PVOID*)&client.srvaddr.sin_addr.s_addr);
#else
    client.srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
#endif
    client.srvaddr.sin_port = htons(DEFAULT_PORT);
    char title[32];
    sprintf(title, "client: %s", ipaddr);
    SetConsoleTitle(title);
    client.sock = socket(AF_INET, SOCK_STREAM, 0);
    BOOL bReuseaddr = TRUE;
    if (client.sock == INVALID_SOCKET) {
        cerr << "socket() invalid with error " << WSAGetLastError() << endl;
        WSACleanup();
        exit(0);
    }
    setsockopt(client.sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(BOOL));
    return 0;
}
string GetLastErrorToString(int errorCode)
{
#ifdef _WIN32
    char *text;
    // 设置FORMAT_MESSAGE_ALLOCATE_BUFFER标志分配内存时需要LocalFree释放
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&text, 0, NULL);
    string result(text);
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
        cerr << "connect() error " << "[" <<
            GetLastErrorToString(WSAGetLastError()).c_str()
            << "] " << endl;
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
    setting.erno = erno;
    if (erno != 0)
        return erno;
    else {
        if (func == NULL) {
            setting.erno = -1;
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
    setting.erno = -1;
    if (!closesocket(client.sock))
        DeleteCriticalSection(&client.wrcon);
    return WSACleanup();
}

int callbackLog(char * usr, char * psw)
{
    trans.uiCmdMsg = 0x1;
    memset(trans.usr, 0, 24);
    memcpy(trans.usr, usr, strlen(usr) + 1);
    memset(trans.psw, 0, 24);
    memcpy(trans.psw, psw, strlen(psw) + 1);
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
int SendChatMsg(st_trans* msg)
{
    if (client.flag < 0) {
        MessageBox(NULL, "Connection error to exit!", "Quit", MB_OK);
        return setting.erno = -1;
    }
    int len = sizeof(trans);
    trans = { '\0', (unsigned char)(trans.uiCmdMsg & 0xff) };
    if (msg != NULL) {
        memcpy(&trans, msg, sizeof(st_trans));
    } else {
        send(client.sock, (char*)&trans, len, 0);
        return -1;
    }
    if (client.last.lastuser != '\0' && client.last.lastgrop[0] != '\0')
    {
        memcpy(trans.usr, client.last.lastuser, 24);
        memcpy(trans.group_name, client.last.lastgrop, 24);
    }
    return send(client.sock, (char*)&trans, len, 0);
}

int GetStatus()
{
    return client.flag;
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
    P2P_NETWORK* P2Psock = (P2P_NETWORK*)PrimaryUDP;
    int len = sizeof(P2Psock->addr);
    for (;;)
    {
        if (client.flag == 0 || P2Psock->socket == NULL)
            continue;
        char recvbuf[256];
        memset(recvbuf, 0, 256);
        int ret = recvfrom(P2Psock->socket, (char*)recvbuf, MAX_PACKET_SIZE, 0, (sockaddr*)&P2Psock->addr,
#ifdef _WIN32
            &len
#else
            (socklen_t*)(&len)
#endif
        );
        if (ret <= 0)
        {
            fprintf(stdout, "Recv Message Error: %s!\n", strerror(ret));
            continue;
        } else {
            int c = 0;
            in_addr peer;
#ifdef _WIN32
            peer.S_un.S_addr = P2Psock->addr.sin_addr.S_un.S_addr;
#else
            peer.s_addr = P2Psock->addr.sin_addr.s_addr;
#endif
            int port = P2Psock->addr.sin_port;
            fprintf(stdout, "Message from [%s:%d] >>\n", inet_ntoa(peer), port);
            fprintf(stdout, "----------------------------------------------------------------\n");
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
}
/*P2P主体函数*/
//流程：首先，直接向某个客户的外网IP发送消息，如果此前没有“打洞”，该消息发送端等待超时；
//判断超时后，发送端发送请求到服务器要求“打洞”，要求服务器请求该客户向本机发送打洞消息。
//重复MAXRETRY次
int p2pMessage(unsigned char *userName, int UserIP, unsigned int UserPort, char const *Message)
{
    SOCKET PrimaryUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (PrimaryUDP < 0)
    {
        std::cout << "UDP socket error!" << std::endl;
        return 0;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(setting.PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int bOptval = 1; // 端口复用
    int retSetsockopt = setsockopt(PrimaryUDP, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptval, sizeof(bOptval));
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
    int MAXRETRY = 5;
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
        memcpy(MessagePeer.usr, userName, 24);
        //发送P2P消息头
        int ppres = sendto(PrimaryUDP, (const char*)&MessagePeer, 32, 0, (const sockaddr*)&remote, sizeof(remote));
        //发送P2P消息体
        ppres = sendto(PrimaryUDP, (const char*)&Message, (int)strlen(Message) + 1, 0, (const sockaddr*)&remote, sizeof(remote));
        //等待接收消息线程修改标志
        for (int i = 0; i < 10; i++)
        {
            if (client.flag <= 0)
                return -1;
            else
                Sleep(300);
        }
        //没有接收到目标主机的回应，认为目标主机的端口
        //映射没有打开，那么发送请求到服务器要求“打洞”。
        st_trans MessageHost;
        memset(&MessageHost, 0, sizeof(st_trans));
        MessageHost.uiCmdMsg = PEER2P;
        memcpy(MessageHost.type, "P2P", 4);
        memcpy(MessageHost.usr, trans.usr, 24);
        memcpy(MessageHost.peer_name, userName, 24);
        //请求服务器“打洞”
        SendChatMsg(&MessageHost);
        //启动接收线程
        P2P_NETWORK pp_sock;
        pp_sock.addr = remote;
        pp_sock.socket = PrimaryUDP;
        Pthreadt threads;
        if (client.count == 0) {
            _beginthreadex(NULL, 0, RecvThreadProc, &pp_sock, 0, &threads);
        }
        client.count++;
        Sleep(100);
    }
    return 0;
}
