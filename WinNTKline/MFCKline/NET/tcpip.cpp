#include "TCPIP.h"

extern bool net_exit;

TCPIP::TCPIP() {}

TCPIP::~TCPIP() {}

unsigned int __stdcall TCPIP::ClientThread(void* pParam)
{
	//线程要调用的函数
	int err;
	int j = 0;
	SOCKET clientSock;
	WSADATA wsaData;//WSAata存储系统传回的关于WinSocket的信息
	TCPIP *m_net = new TCPIP;
	CWnd* m_hWnd = (CWnd*)pParam;
	OGLKview::Point pt = { 0,0 };
	int loo[8];
	CString Str;
	CString Left;
	char Buf[50];
	char buffer[10];
	WCHAR LPCT[50];
	SOCKADDR_IN addrSrv;
	WORD wVersionRequested;
	//连接两个无符号参数
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		::PostMessage(m_hWnd->m_hWnd, WM_MSG_BOX, 0,
			(LPARAM)m_net->Ogl.index.AllocBuffer("服务器可能未启动！"));
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -2;
	}
	clientSock = socket(AF_INET, SOCK_STREAM, 0);//AF_INET 表示TCP 连接
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//本机地址,服务器在本机开启
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6001);//设置端口号
	connect(clientSock, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));//连接服务器
	while (1)
	{
		if (net_exit) { m_hWnd->MessageBox("exit"); return -1; };
		if (recv(clientSock, Buf, 50, 0) == SOCKET_ERROR)
		{
			::PostMessage(m_hWnd->m_hWnd, WM_MSG_BOX, 0,
				(LPARAM)m_net->Ogl.index.AllocBuffer("接收失败！"));
			return -3;
		}//接收数据并填充到列表
		MultiByteToWideChar(CP_ACP, 0, Buf, strlen(Buf) + 1, LPCT, sizeof(LPCT) / sizeof(LPCT[0]));
		m_net->Ogl.DrawKtext(Buf, pt, 20, {1,1,1});
		loo[0] = Str.Find(_T("."));//查找第一个"."位置
		Left = Str.Left(loo[0]);	//将","左边的值取出
		Str.Format(_T("%s"), LPCT);
		_ultoa_s(GetCurrentThreadId(), buffer, 10);//当前线程id
		send(clientSock, buffer, strlen(buffer) + 1, 0);//发送数据
		j++;
	}
	closesocket(clientSock);
	WSACleanup();
	return 0;
}
