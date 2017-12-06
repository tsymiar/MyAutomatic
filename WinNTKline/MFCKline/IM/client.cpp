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
	inet_pton(AF_INET, ipaddr, (PVOID*)&server.sin_addr.s_addr);
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
	struct LPR dlgmsg = { NULL };
	unsigned int thread_ID;
	static char auxstr[24];
	static char sndbuf[256];
	static char rcvbuf[256];
	dlgmsg.sock = rcv;
	dlgmsg.wrcon = wrcon;
	while(info.err == 0)
		if (info.optionum == 0x01) //log
		{
			char title[64];
			sndbuf[0] = 0;
			sndbuf[1] = 0x1;
			strcpy_s(title, "Login successfully as ");
			memcpy(title + 27, info.usr, 24);
			memcpy(sndbuf + 8, info.usr, 24);
			memcpy(sndbuf + 32, info.psw, 24);
			send(out, sndbuf, 256, 0);
			recv(rcv, rcvbuf, 256, 0);
			if (rcvbuf[2] == 0) {
				MessageBox(NULL, title, "Login", MB_OK);
				SetConsoleTitle(title);
				loggedon = 1;
			}
			else
			{
				info.err = -1;
			}
		}
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)func, &dlgmsg, 0, &thread_ID);
	do {
		fflush(stdin);
		onechar = _getch();
		EnterCriticalSection(&wrcon);
		auxstr[0] = onechar;
		auxstr[1] = 0;
		fflush(stdin);

		switch (info.optionum)
		{
		case 0x03: //QUIT
		{
			sndbuf[0] = 0;
			sndbuf[1] = char(3);
			send(out, sndbuf, 256, 0);
			if (rcvbuf[2] == 0) 
				loggedon = 0;
			else
			{
				MessageBox(NULL, "exit error!", "Quit", MB_OK);
				return rcvbuf[1];
			}
			break;
		}
		case 0x04: //HELP
		{
			MessageBox(NULL, "[QIUT]\nto quit program\n[help]\nto show this message\n[list]\nto see online user list\n[allgroup]\nto see group list on this server\n[memberof groupname]\nto see user list of this group\n[setinfo newinfo]\nto set personal info\n[info name]\nto see introduction of a user\n[creategroup groupname grouppassword]\nto create a new group\n[joingroup groupname password]\nto join a group\n[quitgroup groupname]\nto leave a group\n[user/groupname message]\nto talk to a user or group\n[password newpasswrd]\nto set password\n", "HELP message", MB_OK);
			break;
		}
		case 0x05: //LIST
		{
			sndbuf[0] = 0;
			sndbuf[1] = 5;
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x06://"allgroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 6;
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x07: //"memberof"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 7;
			scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x08:// "setinfo"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 8;
			gets_s(sndbuf + 32, 256);
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x09:// "getinfo"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 9;
			scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0A:// "password"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 0x0A;
			scanf_s("%s", (sndbuf + 8), (unsigned)_countof(sndbuf));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0B:// "creategroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 0x0B;
			scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
			strcpy_s((dlgmsg.msg->lastgrop + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0C:// "joingroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 0x0C;
			scanf_s("%s%s", (sndbuf + 8), (unsigned)_countof(sndbuf), (sndbuf + 32), (unsigned)_countof(sndbuf));
			strcpy_s((dlgmsg.msg->lastgrop + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0x0D:// "quitgroup"
		{
			sndbuf[0] = 0;
			sndbuf[1] = 0x0D;
			scanf_s("%s", (sndbuf + 8), 256);
			strcpy_s((dlgmsg.msg->lastgrop + 8), 256, (sndbuf + 8));
			send(out, sndbuf, 256, 0);
			break;
		}
		case 0: break;
		default:
		{
			sndbuf[0] = 0;
			if (info.optionum != 0x00)
				sprintf_s(sndbuf + 1, 4, "%x", info.optionum);
			else
			{
				MessageBox(NULL, "Log failed.", "default", MB_OK);
				return -1;
			}
			memcpy(sndbuf + 8, dlgmsg.msg->lastuser, 24);
			memcpy(sndbuf + 32, dlgmsg.msg->lastgrop, 24);
			send(out, sndbuf, 256, 0);
			break;
		}
		};
		LeaveCriticalSection(&wrcon);
	} while (loggedon == 1);
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

int SetLogInfo(char * usr, char * psw)
{
	memcpy(info.usr, usr, strlen(usr) + 1);
	memcpy(info.psw, psw, strlen(psw) + 1);
	return 0;
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
