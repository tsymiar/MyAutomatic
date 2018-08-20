#include "IMClient.h"

#define DEFAULT_PORT 8877

using namespace std;

MSG_trans trans;
st_client client;
st_settings setting;

//参考该函数编写报文处理函数
void runtime(void* param) {
	static char rcv_buf[256];
	static char srv_net[32];
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
		printf("\nMSG: %s\n", rcv_buf);
		LeaveCriticalSection(&wrcon);
		if (rcvlen <= 0) {
			sprintf(srv_net, "connection lost!\n%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
			MessageBox(NULL, srv_net, itoa(client->sock, rcv_buf, 10), MB_OK);
			if (rcvlen == -1) {
				client->flag = -1;
				closesocket(client->sock);
				break;
			}
			continue;
			'...';
			Sleep(100);
			// exit(0);
		};
	};
};

int InitChat(st_settings* setting) {
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
	if (setting == NULL) {
		setting = &new_sets;
	}
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

unsigned int __stdcall Chat_Msg(void* func)
{
	if (connect(client.sock, (struct sockaddr*)&client.srvaddr, sizeof(client.srvaddr)) == SOCKET_ERROR) {
		cerr << "connect() failed: error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
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
		return _beginthreadex(NULL, 0, Chat_Msg, func, 0, NULL);
	}
}

int CloseChat()
{
	int erno = 0;
	setting.erno = -1;
	if (!closesocket(client.sock))
	{
		erno = WSACleanup();
		DeleteCriticalSection(&client.wrcon);
	}
	return erno;
}

int callbackLog(char * usr, char * psw)
{
	trans.cmd = 0x1;
	memset(trans.usr, 0, 24);
	memcpy(trans.usr, usr, strlen(usr) + 1);
	memset(trans.psw, 0, 24);
	memcpy(trans.psw, psw, strlen(psw) + 1); 
	send(client.sock, (char*)&trans, sizeof(MSG_trans), 0);
	return trans.cmd;
}

int checkPswVilid(char* str)
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

int SetChatMsg(MSG_trans* msg)
{
	if (client.flag < 0) {
		MessageBox(NULL, "exit error!", "Quit", MB_OK);
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
	trans = { '\0', (unsigned char)(trans.cmd & 0xff) };
	if (msg != NULL) {
		memcpy(&trans, msg, sizeof(MSG_trans));
	}
	else {
		send(client.sock, (char*)&trans, len, 0);
		return -1;
	}
	switch (trans.cmd)
	{
	case REGIST:
		break;
	case LOGIN:
	case 2:
	case LOGOUT:
	{
		static char title[64];
		if (trans.usr != NULL) {
			sprintf(title, "Welcome %s", (char*)trans.usr);
			SetConsoleTitle(title);
		}
		send(client.sock, (char*)&trans, 16, 0);
		break;
	}
	case SETPSW:
	{
		gets_s((char*)trans.psw, 24);
		send(client.sock, (char*)&trans, len, 0);
		break;
	}
	case MEMBEROF:
		scanf_s("%s %s", trans.usr, (unsigned)_countof(trans.usr), (trans.grpnm), (unsigned)_countof(trans.grpnm));
		send(client.sock, (char*)&trans, len, 0);
		break;
	case VIEWGROUP:
	{
		scanf_s("%s", trans.grpnm, 24);
		strcpy_s(client.last.lastgrop, 24, (char*)trans.grpnm);
		send(client.sock, (char*)&trans, len, 0);
		break;
	}
	case HOSTGROUP:
	{
		scanf_s("%s %s", trans.hgrp, (unsigned)_countof(trans.hgrp), (trans.grpmrk), (unsigned)_countof(trans.grpmrk));
		strcpy_s(client.last.lastgrop, 24, (char*)trans.hgrp);
		send(client.sock, (char*)&trans, len, 0);
		break;
	}
	case JOINGROUP:
	{
		scanf_s("%s %s", trans.usr, (unsigned)_countof(trans.usr), (trans.jgrp), (unsigned)_countof(trans.jgrp));
		strcpy_s(client.last.lastgrop, 24, (char*)trans.jgrp);
		send(client.sock, (char*)&trans, len, 0);
		break;
	}
	default:
	{
		if (trans.rtn == 0x0)
		{
			MessageBox(NULL, "Logged failure.", "default", MB_OK);
			return -1;
		}
		if (client.last.lastuser && client.last.lastgrop)
		{
			memcpy(trans.usr, client.last.lastuser, 24);
			memcpy(trans.grpnm, client.last.lastgrop, 24);
		}
		send(client.sock, (char*)&trans, 256, 0);
		break;
	}
	}
	return 0;
}
