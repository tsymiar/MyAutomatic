#include "IMClient.h"

#define DEFAULT_PORT 8877

using namespace std;

MSG_trans trans;
st_client client;
st_settings setting;

//参考该函数编写报文处理函数
void runtime(void* param) {
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
            printf("%c", (unsigned char)rcv_buf[c]);
        printf("\n");
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

int InitChat(st_settings* sets) {
    WSADATA wsaData;
    st_settings new_sets;
    static char ipaddr[16];
    memset(ipaddr, 0, 16);
    int erno = WSAStartup(0x202, &wsaData);
    if (erno == SOCKET_ERROR) {
        cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }
    InitializeCriticalSection(&client.wrcon);
    SetConsoleTitle("client v0.1");
    if (sets == NULL) {
        sets = &new_sets;
    }
    if (sets->IP[0] == '\0') {
        strcpy_s(ipaddr, "127.0.0.1");
    }
    else {
        printf_s("Current OS is %d bit.\nNow enter server address: ", sizeof(void*) * 8);
        scanf_s("%s", &ipaddr, (unsigned)_countof(ipaddr));
        if (*ipaddr != 0)
            memcpy(sets->IP, &ipaddr, 16);
        else
            memcpy(&ipaddr, sets->IP, 16);
    };
    memcpy(&setting, sets, sizeof(st_settings));
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
    /*
    SOCKET test = socket(AF_INET, SOCK_STREAM, 0);
    if (test == INVALID_SOCKET) {
        cerr << "socket() failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }
    if (connect(test, (struct sockaddr*)&client.srvaddr, sizeof(client.srvaddr)) == SOCKET_ERROR) {
        cerr << "connect() failed:error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
        WSACleanup();
        return -1;
    }
    closesocket(test);
    */
    client.sock = socket(AF_INET, SOCK_STREAM, 0);
    BOOL bReuseaddr = TRUE;
    if (client.sock == INVALID_SOCKET) {
        cerr << "socket() failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }
    setsockopt(client.sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(BOOL));
    return 0;
}
string GetLastErrorToString(DWORD errorCode)
{
    //设置FORMAT_MESSAGE_ALLOCATE_BUFFER标志分配内存，需要LocalFree释放
    char *text;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&text, 0, NULL);
    string result(text);
    LocalFree(text);
    return result;
}

unsigned int __stdcall Chat_Msg(void* func)
{
    if (connect(client.sock, (struct sockaddr*)&client.srvaddr, sizeof(client.srvaddr)) == SOCKET_ERROR) {
        char *text;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, NULL, WSAGetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&text, 0, NULL);
        cerr << "connect() error " << "[" << WSAGetLastError() << "] " << text << endl;
        WSACleanup();
        return -1;
    }
    unsigned int thread_ID;
    client.flag = 1;
    _beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &client, 0, &thread_ID);
    return 0;
}

int StartChat(int erno, void(*func)(void*))
{
    setting.erno = erno;
    if (erno != 0)
        return erno;
    else {
        if (func == NULL)
            func = [](void*) {printf("Lambda null func.\n"); };
        return (int)_beginthreadex(NULL, 0, Chat_Msg, func, 0, NULL);
    }
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
    send(client.sock, (char*)&trans, sizeof(MSG_trans), 0);
    return trans.uiCmdMsg;
}

int checkPswValid(char* str)
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
        }
        else if (ansi <= 'z' && ansi >= 'a')
        {
            zz = 1;
        }
        else if (ansi <= 'Z' && ansi >= 'A')
        {
            zZ = 1;
        }
        else if (ansi > 127)
        {
            z_ = 0;
        }
        else
        {
            z_ = 1;
        }
    }
    return (z0 + zz + zZ + z_ == 4 ? 1 : 0);
}
/*P2P接收消息线程*/
unsigned int __stdcall RecvThreadProc(void* PrimaryUDP)
{
    int MAX_PACKET_SIZE = 256;
    PPSOCK* P2Psock = (PPSOCK*)PrimaryUDP;
    int len = sizeof(P2Psock->addr);
    for (;;)
    {
        char recvbuf[256];
        if (client.flag == 0 || P2Psock->sock == NULL)
            continue;
        int ret = recvfrom(P2Psock->sock, (char*)recvbuf, MAX_PACKET_SIZE, 0, (sockaddr*)&P2Psock->addr, &len);
        if (ret <= 0)
        {
            printf("Recv Message Error: %s!\n", strerror(ret));
            continue;
        }
        else {
            int c = 0;
            in_addr peer;
            peer.S_un.S_addr = P2Psock->addr.sin_addr.S_un.S_addr;
            int port = P2Psock->addr.sin_port;
            printf("Message from [%s:%d] >>\n", inet_ntoa(peer), port);
            printf("----------------------------------------------------------------\n");
            for (c = 0; c < ret; c++)
            {
                if (c > 0 && c % 32 == 0)
                    printf("\n");
                printf("%02x ", (unsigned char)recvbuf[c]);
            }
            printf("\n");
            for (c = 0; c < ret; c++)
                printf("%c", (unsigned char)recvbuf[c]);
            printf("\n");
        }
    }
}
/*P2P主体函数*/
//流程：首先，直接向某个客户的外网IP发送消息，如果此前没有“打洞”，该消息发送端等待超时；
//判断超时后，发送端发送请求到服务器要求“打洞”，要求服务器请求该客户向本机发送打洞消息。
//重复MAXRETRY次
int p2pMessage(char *userName, int UserIP, unsigned int UserPort, const char *Message)
{
    int MAXRETRY = 5;
    int P2PMESSAGE = 100;
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

    for (int trytime = 0; trytime < MAXRETRY; trytime++)
    {
        sockaddr_in remote;
        remote.sin_family = AF_INET;
        remote.sin_port = htons(UserPort);
        remote.sin_addr.S_un.S_addr = htonl(UserIP);

        MSG_trans MessageHead;
        memset(&MessageHead, 0, sizeof(MessageHead));
        MessageHead.uiCmdMsg = P2PMESSAGE;
        memcpy(MessageHead.usr, userName, 24);
        //发送P2P消息头
        int ppres = sendto(PrimaryUDP, (const char*)&MessageHead, 32, 0, (const sockaddr*)&remote, sizeof(remote));
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
        MSG_trans msg;
        memset(&msg, 0, sizeof(MSG_trans));
        msg.uiCmdMsg = CHATWITH;
        memcpy(msg.chk, "P2P", 4);
        memcpy(msg.usr, trans.usr, 24);
        memcpy(msg.peer, userName, 24);
        //请求服务器“打洞”
        SendChatMsg(&msg);
        PPSOCK pp_sock;
        pp_sock.addr = remote;
        pp_sock.sock = PrimaryUDP;
        if (client.count == 0)
            _beginthreadex(NULL, 0, RecvThreadProc, &pp_sock, 0, NULL);
        client.count++;
        Sleep(100);
    }
    return 0;
}

int SendChatMsg(MSG_trans* msg)
{
    if (client.flag < 0) {
        MessageBox(NULL, "Connection error to exit!", "Quit", MB_OK);
        return setting.erno = -1;
    }
#ifdef TEST_SOCK
    static char auxstr[256];
    /*fflush(stdin);
    char onechar = _getch();
    auxstr[0] = onechar;
    auxstr[1] = 0;
    fflush(stdin);*/
    int k = 0;
    if (k == 0)
    {
        memset(auxstr, 0, 256);
        send(client.sock, auxstr, 256, 0);
        k++;
        if (k == INT_MAX)
            k = 1;
    }
    // rcvlen = recv(client.sock, rcvbuf, 256, 0);
#endif
    int len = sizeof(trans);
    trans = { '\0', (unsigned char)(trans.uiCmdMsg & 0xff) };
    if (msg != NULL) {
        memcpy(&trans, msg, sizeof(MSG_trans));
    }
    else {
        send(client.sock, (char*)&trans, len, 0);
        return -1;
    }
    if (client.last.lastuser != '\0' && client.last.lastgrop[0] != '\0')
    {
        memcpy(trans.usr, client.last.lastuser, 24);
        memcpy(trans.grpnm, client.last.lastgrop, 24);
    }
    return send(client.sock, (char*)&trans, len, 0);
}

int GetStatus()
{
    return client.flag;
}
