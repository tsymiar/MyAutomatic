#include "IMClient.h"

#define DEFAULT_PORT 8877

using namespace std;

st_imusr info;
SOCKET rcvsock;
int logstatus = 0;
clientsocket client_sock;

//参考该函数编写报文处理函数
void runtime(void* lp) {
	int feedback;
	static char buff[256];
	clientsocket* msg = (clientsocket*)lp;
	do {
		feedback = recv(msg->sock, buff, 256, 0);
		if (!(feedback == 256)) {
			Sleep(100);
			MessageBox(NULL, "connection lost.", "client", MB_OK);
			exit(0);
		};
		EnterCriticalSection(&msg->wrcon);
		'...';
		LeaveCriticalSection(&msg->wrcon);
	} while (1);
};

int InitChat(st_imusr* imusr) {
	WSADATA wsaData;
	static char ipaddr[16];
	int err = WSAStartup(0x202, &wsaData);
	if (err == SOCKET_ERROR) {
		cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	InitializeCriticalSection(&client_sock.wrcon);
	SetConsoleTitle("chat client");
	if (imusr->IP[0] == '\0') {
		strcpy_s(ipaddr, "127.0.0.1");
	}
	else {
			printf_s("enter server address:");
			scanf_s("%s", &ipaddr, (unsigned)_countof(ipaddr));
			memcpy(&ipaddr, imusr->IP, 16);
	};
	client_sock.srvaddr.sin_family = AF_INET;
#ifdef _UTILAPIS_
	inet_pton(AF_INET, ipaddr, (PVOID*)&client_sock.srvaddr.sin_addr.s_addr);
#else
	client_sock.srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
#endif
	client_sock.srvaddr.sin_port = htons(DEFAULT_PORT);
	SOCKET test = socket(AF_INET, SOCK_STREAM, 0);
	if (test == INVALID_SOCKET) {
		cerr << "socket() failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	if (connect(test, (struct sockaddr*)&client_sock.srvaddr, sizeof(client_sock.srvaddr)) == SOCKET_ERROR) {
		cerr << "connect() failed:error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	closesocket(test);
	client_sock.sock = socket(AF_INET, SOCK_STREAM, 0);
	BOOL bReuseaddr = TRUE;
	setsockopt(client_sock.sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(BOOL));
	if (client_sock.sock == INVALID_SOCKET) {
		cerr << "socket() failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	rcvsock = socket(AF_INET, SOCK_STREAM, 0);
	if (rcvsock == INVALID_SOCKET) {
		cerr << "socket() failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	return 0;
}

unsigned int __stdcall Chat_Msg(void* func)
{
	int k = 0;
	int rcvlen, curlen = 0;
	unsigned int thread_ID;
	static char auxstr[24];
	static char title[64];
	static char sndbuf[256];
	static char rcvbuf[256];
	if (connect(rcvsock, (struct sockaddr*)&client_sock.srvaddr, sizeof(client_sock.srvaddr)) == SOCKET_ERROR) {
		cerr << "connect() failed:error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	clientsocket imsock;
	imsock.sock = socket(AF_INET, SOCK_STREAM, 0);
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &imsock, 0, &thread_ID);
	while (info.err == 0)
	{
		EnterCriticalSection(&client_sock.wrcon);
		/*fflush(stdin);
		char onechar = _getch();
		auxstr[0] = onechar;
		auxstr[1] = 0;
		fflush(stdin);*/
		if (k == 0)
		{
			memset(sndbuf, 0, 256);
			send(client_sock.sock, sndbuf, 256, 0);
			k++;
			if (k == INT_MAX)
				k = 1;
		}
		rcvlen = recv(rcvsock, rcvbuf, 256, 0);
		sndbuf[0] = 0;
		sndbuf[1] = info.option;
		switch (sndbuf[1])
		{
		case 0:
			continue;
		case 1:
			if (info.usr != NULL)
				strcpy_s(title, info.usr);
			if (rcvbuf[2] == 0x30) {
				if (curlen != rcvlen && rcvlen != 0)
					SetConsoleTitle(title);
				logstatus = 1;
				continue;
			}
			break;
		case 0x03: //QUIT
		{
			send(client_sock.sock, sndbuf, 256, 0);
			if (rcvbuf[2] == 0x30)
			{
				logstatus = 0;
				break;
			}
			else
			{
				MessageBox(NULL, "exit error!", "Quit", MB_OK);
				return info.err = -1;
			}
		}
		case 0x05: //LIST
		{
			if (logstatus)
			{
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x06://"ALL"
		{
			if (logstatus)
			{
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x07: //"memberof"
		{
			if (logstatus)
			{
				scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x08:// "HOST"
		{
			if (logstatus)
			{
				scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
				strcpy_s((client_sock.msg->lastgrop + 8), 256, (sndbuf + 8));
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x09:// "JOIN"
		{
			if (logstatus)
			{
				scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
				strcpy_s((client_sock.msg->lastgrop + 8), 256, (sndbuf + 8));
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x0A:// "quit"
		{
			if (logstatus)
			{
				scanf_s("%s", (sndbuf + 8), 256);
				strcpy_s((client_sock.msg->lastgrop + 8), 256, (sndbuf + 8));
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x0B:
		{
			if (logstatus)
			{
				gets_s(sndbuf + 32, 256);
				send(client_sock.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		default:
		{
			if (info.option == 0x0)
			{
				MessageBox(NULL, "Logged failed.", "default", MB_OK);
				return -1;
			}
			if (client_sock.msg->lastuser && client_sock.msg->lastgrop)
			{
				memcpy(sndbuf + 8, client_sock.msg->lastuser, 24);
				memcpy(sndbuf + 32, client_sock.msg->lastgrop, 24);
			}
			send(client_sock.sock, sndbuf, 256, 0);
			break;
		}
		};
		curlen = rcvlen;
		LeaveCriticalSection(&client_sock.wrcon);
	}
	return 0;
}

int StartChat(int err, void(*func)(void*))
{
	if (err != 0)
		return err;
	else {
		if (func == NULL)
			return -1;
		else
			return _beginthreadex(NULL, 0, Chat_Msg, func, 0, NULL);
	}
}

int SetOptCmd(unsigned int cmd)
{
	return(info.option = cmd);
}

int transMsg(char* msg)
{
	if (connect(client_sock.sock, (struct sockaddr*)&client_sock.srvaddr, sizeof(client_sock.srvaddr)) == SOCKET_ERROR) {
		cerr << "connect() failed:error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	return send(client_sock.sock, msg, 256, 0);
}

int SetLogInfo(char * usr, char * psw)
{
	char logmsg[64] = { 0,0x01 };
	memset(info.usr, 0, 24);
	memset(info.psw, 0, 24);
	memcpy(info.usr, usr, strlen(usr) + 1);
	memcpy(info.psw, psw, strlen(psw) + 1);
	memcpy(logmsg + 8, info.usr, 24);
	memcpy(logmsg + 32, info.psw, 24);
	send(client_sock.sock, logmsg, 64, 0);
	return logstatus;
}

int SetStatus()
{
	return (logstatus = 0);
}

int CloseChat()
{
	int err = 0;
	info.err = -1;
	if (!closesocket(rcvsock) && !closesocket(client_sock.sock))
	{
		err = WSACleanup();
		DeleteCriticalSection(&client_sock.wrcon);
	}
	return err;
}
