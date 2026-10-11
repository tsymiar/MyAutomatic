#include "CTPclient.h"
#include "ChannelCollector.h"

extern bool net_exit;
struct st_TThostFtdc STFTDC;
CThostFtdcTraderApi *TRDAPI;

CTPclient::CTPclient() {}

CTPclient::~CTPclient() {}

unsigned int __stdcall CTPclient::SimpleClient(void* P)
{
    //线程要调用的函数
    int err;
    SOCKET clientSock = INVALID_SOCKET;
    WSADATA wsaData;//WSAata存储系统传回的关于WinSocket的信息
    MyOglDrawDlg* m_hWnd = reinterpret_cast<MyOglDrawDlg*>(P);
    OGLKview::Point pnt = { 0,0 };
    CString Str;
    CString Left;
    char Buf[50];
    char buffer[16];
    WCHAR LPCT[50] = { 0 };
    SOCKADDR_IN addrSrv;

    // 线程参数可能为空
    if (m_hWnd == NULL || m_hWnd->ctpIP == NULL)
        return (unsigned int)-1;

    CTPclient ctp;                    // 栈对象
    //连接两个无符号参数
    WORD wVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        ::PostMessage(m_hWnd->m_hWnd, WM_MSG_BOX, 0,
            (LPARAM)ctp.Ogl.conv.AllocBuffer("服务器可能未启动！"));
        return (unsigned int)-1;
    }
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
    {
        WSACleanup();
        return (unsigned int)-2;
    }
    clientSock = socket(AF_INET, SOCK_STREAM, 0);//AF_INET 表示TCP 连接
    if (clientSock == INVALID_SOCKET)
    {
        WSACleanup();
        return (unsigned int)-3;
    }
    addrSrv.sin_addr.S_un.S_addr = inet_addr((const char*)m_hWnd->ctpIP);//本机地址,服务器在本机开启
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(6001);//设置端口号
    if (connect(clientSock, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)//连接服务器
    {
        ::PostMessage(m_hWnd->m_hWnd, WM_MSG_BOX, 0,
            (LPARAM)ctp.Ogl.conv.AllocBuffer("连接服务器失败！"));
        closesocket(clientSock);
        WSACleanup();
        return (unsigned int)-4;
    }
    while (!net_exit)
    {
        int recvLen = recv(clientSock, Buf, (int)sizeof(Buf) - 1, 0);
        if (recvLen == SOCKET_ERROR || recvLen == 0)//接收数据并填充到列表
        {
            if (m_hWnd->m_hWnd != NULL)
                ::PostMessage(m_hWnd->m_hWnd, WM_MSG_BOX, 0,
                    (LPARAM)ctp.Ogl.conv.AllocBuffer("接收失败！"));
            closesocket(clientSock);
            WSACleanup();
            return (unsigned int)-5;
        }
        // recv 不保证以 '\0' 结尾，显式截断后再按有界长度转换
        Buf[recvLen] = '\0';
        MultiByteToWideChar(CP_ACP, 0, Buf, recvLen + 1, LPCT, sizeof(LPCT) / sizeof(LPCT[0]));
        ctp.Ogl.DrawKtext(Buf, pnt, 20, { 1,1,1 });
        // 先 Format 再 Find
        Str.Format(_T("%s"), LPCT);
        int dot = Str.Find(_T("."));//查找第一个"."位置
        if (dot > 0)
            Left = Str.Left(dot);    //将"."左边的值取出
        _ultoa_s(GetCurrentThreadId(), buffer, sizeof(buffer), 10);//当前线程id
        send(clientSock, buffer, (int)strnlen(buffer, sizeof(buffer)) + 1, 0);//发送数据
    }
    closesocket(clientSock);
    WSACleanup();
    return 0;
}

unsigned int __stdcall CTPclient::TradeMarket(void* P)
{
    MyOglDrawDlg* m_hWnd = reinterpret_cast<MyOglDrawDlg*>(P);
    if (AllocConsole() && freopen("CONOUT$", "w", stderr)) {}
    TRDAPI = CThostFtdcTraderApi::CreateFtdcTraderApi(STFTDC.FLOW_PATH);
    if (TRDAPI == NULL)
        return (unsigned int)-1;
    TradeChannel tdChnl;             // 栈对象
    TRDAPI->RegisterSpi((CThostFtdcTraderSpi*)&tdChnl);
    TRDAPI->RegisterFront(STFTDC.FRONT_ADDR);
    TRDAPI->SubscribePrivateTopic(THOST_TERT_QUICK);
    TRDAPI->SubscribePublicTopic(THOST_TERT_QUICK);
    TRDAPI->Init();
    TRDAPI->Join();
    CString stret;
    stret.Format(_T("%d"), tdChnl.getRtn());
    // 参数可能为空
    if (m_hWnd != NULL && m_hWnd->m_hWnd != NULL)
    {
        CTPclient ctp;
        ::PostMessage(m_hWnd->m_hWnd, WM_MSG_BOX, 0,
            (LPARAM)ctp.Ogl.conv.AllocBuffer(stret));
    }
    return 0;
}
