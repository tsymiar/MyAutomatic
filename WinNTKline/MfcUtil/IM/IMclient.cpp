#include "IMClient.h"

#define DEFAULT_PORT 8877

using namespace std;

MSG_trans trans;
st_client client;
st_settings setting;

//参考该函数编写报文处理函数
void runtime(void* lp) {
	int rcvlen;
	static char buff[256];
	static char srv_net[32];
	CRITICAL_SECTION wrcon;
	memset(buff, 0, 256);
	InitializeCriticalSection(&wrcon);
	st_client* client = reinterpret_cast<st_client*>(lp);
	do {
		rcvlen = recv(client->sock, buff, 256, 0);
		if (rcvlen <= 0) {
			Sleep(100);
			sprintf(srv_net, "%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
			MessageBox(NULL, "connection lost!", srv_net, MB_OK);
			closesocket(client->sock);
			// exit(0);
		};
		EnterCriticalSection(&wrcon);
		'...';
		LeaveCriticalSection(&wrcon);
	} while (1);
};

int InitChat(st_settings* setting) {
	WSADATA wsaData;
	static char ipaddr[16];
	memset(ipaddr, 0, 16);
	int err = WSAStartup(0x202, &wsaData);
	if (err == SOCKET_ERROR) {
		cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	InitializeCriticalSection(&client.wrcon);
	SetConsoleTitle("client v0.1");
	if (setting->IP[0] == '\0') {
		strcpy_s(ipaddr, "127.0.0.1");
	}
	else {
		printf_s("Now enter server address: ");
		scanf_s("%s", &ipaddr, (unsigned)_countof(ipaddr));
		if (*ipaddr != 0)
			memcpy(setting->IP, &ipaddr, 16);
		else
			memcpy(&ipaddr, setting->IP, 16);
	};
	client.srvaddr.sin_family = AF_INET;
#ifdef _UTILAPIS_
	inet_pton(AF_INET, ipaddr, (PVOID*)&client.srvaddr.sin_addr.s_addr);
#else
	client.srvaddr.sin_addr.s_addr = inet_addr(ipaddr);
#endif
	client.srvaddr.sin_port = htons(DEFAULT_PORT);
	char title[32];
	sprintf(title, "client of %s", ipaddr);
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

unsigned int __stdcall Chat_Msg(void* func)
{
	if (connect(client.sock, (struct sockaddr*)&client.srvaddr, sizeof(client.srvaddr)) == SOCKET_ERROR) {
		cerr << "connect() failed: error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	unsigned int thread_ID;
	st_client imclient;
	static char title[64];
	static char sndbuf[256];
	imclient.sock = client.sock;// socket(AF_INET, SOCK_STREAM, 0);
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &imclient, 0, &thread_ID);
	while (setting.err == 0)
	{
		EnterCriticalSection(&client.wrcon);
#ifdef TEST_SOCK
		static char auxstr[24];
		/*fflush(stdin);
		char onechar = _getch();
		auxstr[0] = onechar;
		auxstr[1] = 0;
		fflush(stdin);*/
		int k = 0;
		if (k == 0)
		{
			memset(sndbuf, 0, 256);
			send(client.sock, sndbuf, 256, 0);
			k++;
			if (k == INT_MAX)
				k = 1;
		}
		// rcvlen = recv(client.sock, rcvbuf, 256, 0);
#endif
		sndbuf[0] = 0;
		sndbuf[1] = setting.option;
		switch (sndbuf[1])
		{
		case 0:
			continue;
		case 0x1:
			if (trans.usr != NULL) {
				strcpy_s(title, (char*)trans.usr);
				SetConsoleTitle(title);
				// client.status = 1;
				continue;
			}
			break;
		case 0x03: //QUIT
		{
			send(client.sock, sndbuf, 256, 0);
			if (client.status)
			{
				MessageBox(NULL, "exit error!", "Quit", MB_OK);
				return setting.err = -1;
			}
		}
		case 0x05: //LIST
		{
			if (client.status)
			{
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x06://"ALL"
		{
			if (client.status)
			{
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x07: //"memberof"
		{
			if (client.status)
			{
				scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x08:// "HOST"
		{
			if (client.status)
			{
				scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
				strcpy_s((client.last->lastgrop + 8), 256, (sndbuf + 8));
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x09:// "JOIN"
		{
			if (client.status)
			{
				scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
				strcpy_s((client.last->lastgrop + 8), 256, (sndbuf + 8));
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x0A:// "QUIT"
		{
			if (client.status)
			{
				scanf_s("%s", (sndbuf + 8), 256);
				strcpy_s((client.last->lastgrop + 8), 256, (sndbuf + 8));
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		case 0x0B:
		{
			if (client.status)
			{
				gets_s(sndbuf + 32, 256);
				send(client.sock, sndbuf, 256, 0);
				break;
			}
			else continue;
		}
		default:
		{
			if (setting.option == 0x0)
			{
				MessageBox(NULL, "Logged failed.", "default", MB_OK);
				return -1;
			}
			if (client.last->lastuser && client.last->lastgrop)
			{
				memcpy(sndbuf + 8, client.last->lastuser, 24);
				memcpy(sndbuf + 32, client.last->lastgrop, 24);
			}
			send(client.sock, sndbuf, 256, 0);
			break;
		}
		};
		LeaveCriticalSection(&client.wrcon);
	}
	return 0;
}

int StartChat(int err, void(*func)(void*))
{
	setting.err = err;
	if (err != 0)
		return err;
	else {
		if (func == NULL)
			return -1;
		else
			return _beginthreadex(NULL, 0, Chat_Msg, func, 0, NULL);
	}
}

int SetCommond(unsigned int cmd)
{
	return(trans.cmd = cmd);
}

int SetChatMsg(MSG_trans* msg)
{
	return send(client.sock, (char*)&msg, 256, 0);
}

int SetLogInfo(char * usr, char * psw)
{
	memset(trans.usr, 0, 24);
	memcpy(trans.usr, usr, strlen(usr) + 1);
	memset(trans.psw, 0, 24);
	memcpy(trans.psw, psw, strlen(psw) + 1);
	send(client.sock, (char*)&trans, 64, 0);
	return client.status;
}

int SetStatus(int t)
{
	return (client.status = t);
}

int CloseChat()
{
	int err = 0;
	setting.err = -1;
	if (!closesocket(client.sock))
	{
		err = WSACleanup();
		DeleteCriticalSection(&client.wrcon);
	}
	return err;
}
