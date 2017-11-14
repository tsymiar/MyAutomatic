#include "IMClient.h"

#define DEFAULT_PORT 8877

using namespace std;

SOCKET rcv, out;
int loggedon = 0;
CRITICAL_SECTION wrcon;
st_imusr info;

//参考该函数编写报文处理函数
void runtime(void* lp) {
	int feedback;
	static char cmd[256];
	struct LPR* lpr = (struct LPR*)lp;
	do {
		feedback = recv(lpr->sock, cmd, 256, 0);
		if (!(feedback == 256)) {
			Sleep(100);
			printf("connection lost.\n");
			exit(0);
		};
		EnterCriticalSection(&lpr->wrcon);
		'...';
		LeaveCriticalSection(&lpr->wrcon);
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
	InitializeCriticalSection(&wrcon);
	SetConsoleTitle("chat client");
	if (imusr == NULL) {
		strcpy_s(ipaddr, "127.0.0.1");
	}
	else {
		if (imusr->addr[0] == NULL) 
			strcpy_s(ipaddr, "127.0.0.1");
		else
		{
			printf_s("enter server address:");
			scanf_s("%s", &ipaddr, (unsigned)_countof(ipaddr));
		}
	};
	struct sockaddr_in server;
	server.sin_family = AF_INET;
#ifdef _TESTAPIS_
	inet_pton(AF_INET,ipaddr, (PVOID*)&server.sin_addr.s_addr);
#else
	server.sin_addr.s_addr = inet_addr(ipaddr);
#endif
	server.sin_port = htons(DEFAULT_PORT);
	out = socket(AF_INET, SOCK_STREAM, 0);
	if (out == INVALID_SOCKET) {
		cerr << "socket() failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	if (connect(out, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
		cerr << "connect() failed:error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	rcv = socket(AF_INET, SOCK_STREAM, 0);
	if (out == INVALID_SOCKET) {
		cerr << "socket() failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	if (connect(rcv, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
		cerr << "connect() failed:error " << "[" << WSAGetLastError() << "] " << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	return 0;
}

unsigned int __stdcall Chat_Msg(void* func)
{
	char onechar;
	struct LPR lpr;
	unsigned int thread_ID;
	static char auxstr[24];
	static char sndbuf[256];
	while(info.err==0)
	switch (info.optionum)
	{
	case 0x01: //Regist
	{
		sndbuf[0] = 0;
		sndbuf[1] = 0;
		printf("name want to use:");
		scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
		printf("password:");
		scanf_s("%s", (sndbuf + 32), (unsigned)_countof(sndbuf));
		send(out, sndbuf, 256, 0);
		recv(rcv, sndbuf, 256, 0);
		if (sndbuf[1] == 0)
			MessageBox(NULL, "Regist succesfully.", "Regist", MB_OK);
		else
		{
			MessageBox(NULL, "Regist failed.", "Regist", MB_OK);
		}
	}
	case (0x02): //log
	{
		sndbuf[0] = 0;
		sndbuf[1] = (char)120;
		printf("user name:");
		scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
		char title[30];
		strcpy_s(title, "chat client, logged on as ");
		strcat_s(title, (sndbuf + 8));
		printf("password:");
		scanf_s("%s", (sndbuf + 32), 256);
		send(out, sndbuf, 256, 0);
		recv(rcv, sndbuf, 256, 0);
		if (sndbuf[1] == 120) {
			MessageBox(NULL, "Logged on successfully.", "Log", MB_OK);
			SetConsoleTitle(title);
			loggedon = 1;
		}
		else
			MessageBox(NULL, "Log on failed.", "Log", MB_OK);
	}
	case 0x00: break;
	default:
		MessageBox(NULL, "Please set a valid commond.", "Log", MB_OKCANCEL);
		break;
	}
	lpr.sock = rcv;
	lpr.wrcon = wrcon;
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &lpr, 0, &thread_ID);
	do {
		fflush(stdin);
		onechar = _getch();
		EnterCriticalSection(&wrcon);

		auxstr[0] = onechar;
		auxstr[1] = 0;
		fflush(stdin);
		switch (info.optionum)
		{
		case 0x03: {//QUIT
			sndbuf[0] = 0;
			sndbuf[1] = (char)121;
			send(out, sndbuf, 256, 0);
			loggedon = 0;
			break;
		}
		case 0x04: //HELP
		{
			MessageBox(NULL, "[QIUT]\nto quit program\n[help]\nto show this message\n[list]\nto see online user list\n[allgroup]\nto see group list on this server\n[memberof groupname]\nto see user list of this group\n[setinfo newinfo]\nto set personal info\n[info name]\nto see introduction of a user\n[creategroup groupname grouppassword]\nto create a new group\n[joingroup groupname password]\nto join a group\n[quitgroup groupname]\nto leave a group\n[user/groupname message]\nto talk to a user or group\n[password newpasswrd]\nto set password\n", "HELP message", MB_OK);
			break;
		}
		case 0x05: {//LIST
			sndbuf[0] = 0;
			sndbuf[1] = 20;
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x06://"allgroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 15;
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x07: //"memberof"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 16;
			scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x08:// "setinfo"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 1;
			gets_s(sndbuf + 32, 256);
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x09:// "getinfo"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 21;
			scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0A:// "password"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 122;
			scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0B:// "creategroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 11;
			scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
			strcpy_s((lpr.msg->lastgroup + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0C:// "joingroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 10;
			scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
			strcpy_s((lpr.msg->lastgroup + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0D:// "quitgroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 12;
			scanf_s("%s", (sndbuf + 8), 256);
			strcpy_s((lpr.msg->lastgroup + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0: break;
		default: 
		{
			sndbuf[0] = 0;
			sndbuf[1] = 30;
			if (info.optionum != 0x00)
				sprintf_s(sndbuf + 8, 24, "%x", info.optionum);
			gets_s(sndbuf + 32, 256);
			strcpy_s((lpr.msg->lastuser + 32), 256, (sndbuf + 32));
			strcpy_s((lpr.msg->lastuser + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		};
		LeaveCriticalSection(&wrcon);
	} while (loggedon == 1);
	printf("quit now\n");
	return 0;
}

int StartChat(int err, void(*func)(void*))
{
	if (err != 0)
		return err;
	else
		return _beginthreadex(NULL, 0, Chat_Msg, func, 0, NULL);
}

int SetChatCmd(unsigned int cmd)
{
	return(info.optionum = cmd);
}

int CloseChat()
{
	int err = 0;
	if (!closesocket(rcv) && !closesocket(out))
	{
		err = WSACleanup();
		DeleteCriticalSection(&wrcon);
	}
	return err;
}
