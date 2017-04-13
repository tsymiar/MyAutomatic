#include "IMClient.h"

using namespace std;
#define DEFAULT_PORT 8877

SOCKET rcv, out;
int loggedon = 0;
CRITICAL_SECTION wrcon;

//参考该函数编写报文处理函数
void runtime(void* lp) {
	int feedback;
	char cmd[256];
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

int InitChat(char argv[], int argc) {
	WSADATA wsaData;
	char ipaddr[16];
	int err = WSAStartup(0x202, &wsaData);
	if (err == SOCKET_ERROR) {
		cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	InitializeCriticalSection(&wrcon);
	SetConsoleTitle("chat client");
	if (argc == 2) {
		strcpy_s(ipaddr, argv/*[1]*/);
	}
	else {
		printf_s("enter server address:");
		scanf_s("%s", &ipaddr, (unsigned)_countof(ipaddr));
	};
	struct sockaddr_in server;
	server.sin_family = AF_INET;
#ifdef _TESTDLG_
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
	char auxstr[24];
	char Buffer[256];
	int optionum = 0x00;
	do {
		MessageBox(NULL, "Connect to server OK, [login] or [regist] a new account? (l/r):","MESSAGE", MB_OK);
		switch (optionum)
		{
		case 0x01: {//log
			Buffer[0] = 0;
			Buffer[1] = (char)120;
			printf("user name:");
			scanf_s("%s", (Buffer + 8), (unsigned)_countof(Buffer));
			char title[30];
			strcpy_s(title, "chat client,logged on as ");
			strcat_s(title, (Buffer + 8));
			printf("password:");
			scanf_s("%s", (Buffer + 32), 256);
			send(out, Buffer, 256, 0);
			recv(rcv, Buffer, 256, 0);
			if (Buffer[1] == 120) {
				printf("logged on successfully\n");
				SetConsoleTitle(title);
				loggedon = 1;
			}
			else
				printf("log on failed\n");
		} break;
		case (0x02): {//regist
			Buffer[0] = 0;
			Buffer[1] = 0;
			printf("name want to use:");
			scanf_s("%s", (Buffer + 8), (unsigned)_countof(Buffer));
			printf("password:");
			scanf_s("%s", (Buffer + 32), (unsigned)_countof(Buffer));
			send(out, Buffer, 256, 0);
			recv(rcv, Buffer, 256, 0);
			if (Buffer[1] == 0)
				printf("registered succesfully\n");
			else
				printf("register failed\n");
		} break;
		default:
			printf("please in put a valid option\n");
			break;
		}
	} while (loggedon == 0);
	printf("type command [help] to see help message\n");
	lpr.sock = rcv;
	lpr.wrcon = wrcon;
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)(func), &lpr, 0, &thread_ID);
	do {
		fflush(stdin);
		onechar = _getch();
		EnterCriticalSection(&wrcon);

		auxstr[0] = onechar;
		auxstr[1] = 0;
		fflush(stdin);
		//scanf_s("%s", optionstr, (unsigned)_countof(optionstr));
		// strcat(auxstr,optionstr);
		// strcpy(optionstr,auxstr);
		switch (optionum)
		{
		case 0x03: {//QUIT
			Buffer[0] = 0;
			Buffer[1] = (char)121;
			send(out, Buffer, 256, 0);
			loggedon = 0;
			break;
		}
		case 0x04: {/*//HELP
												  printf("\nyes,let me help you\n");
												  printf("quit to quit program\nhelp to show this message\nlist to see online user list\nallgroup to see group list on this server\n");
												  printf("memberof groupname to see user list of this group\nsetinfo newinfo to set personal info\ninfo name  to see introduction of a user\ncreategroup groupname grouppassword to create a new group\n");
												  printf("joingroup groupname password to join a group\nquitgroup groupname to leave a group\n");
												  printf("user/groupname message to talk to a user or group\npassword newpasswrd to set password\n\n");
												  */
			MessageBox(NULL, "[quit]\nto quit program\n[help]\nto show this message\n[list]\nto see online user list\n[allgroup]\nto see group list on this server\n[memberof groupname]\nto see user list of this group\n[setinfo newinfo]\nto set personal info\n[info name]\nto see introduction of a user\n[creategroup groupname grouppassword]\nto create a new group\n[joingroup groupname password]\nto join a group\n[quitgroup groupname]\nto leave a group\n[user/groupname message]\nto talk to a user or group\n[password newpasswrd]\nto set password\n", "help message", MB_OK);
			break;
		}
		case 0x05: {//LIST
			Buffer[0] = 0;
			Buffer[1] = 20;
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x06://"allgroup"
		{
			Buffer[0] = 0;
			Buffer[1] = 15;
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x07: //"memberof"
		{
			Buffer[0] = 0;
			Buffer[1] = 16;
			scanf_s("%s", (Buffer + 8), (unsigned)_countof(Buffer));
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x08:// "setinfo"
		{
			Buffer[0] = 0;
			Buffer[1] = 1;
			gets_s(Buffer + 32, 256);
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x09:// "info"
		{
			Buffer[0] = 0;
			Buffer[1] = 21;
			scanf_s("%s", (Buffer + 8), (unsigned)_countof(Buffer));
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x0A:// "password"
		{
			Buffer[0] = 0;
			Buffer[1] = 122;
			scanf_s("%s", (Buffer + 8), (unsigned)_countof(Buffer));
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x0B:// "creategroup"
		{
			Buffer[0] = 0;
			Buffer[1] = 11;
			scanf_s("%s%s", (Buffer + 8), (unsigned)_countof(Buffer), (Buffer + 32), (unsigned)_countof(Buffer));
			strcpy_s((lpr.msg->lastgroup + 8), 256, (Buffer + 8));
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x0C:// "joingroup"
		{
			Buffer[0] = 0;
			Buffer[1] = 10;
			scanf_s("%s%s", (Buffer + 8), (unsigned)_countof(Buffer), (Buffer + 32), (unsigned)_countof(Buffer));
			strcpy_s((lpr.msg->lastgroup + 8), 256, (Buffer + 8));
			send(out, Buffer, 256, 0);
			break;
		}
		case 0x0D:// "quitgroup"
		{
			Buffer[0] = 0;
			Buffer[1] = 12;
			scanf_s("%s", (Buffer + 8), 256);
			strcpy_s((lpr.msg->lastgroup + 8), 256, (Buffer + 8));
			send(out, Buffer, 256, 0);
			break;
		}
		default: {
			Buffer[0] = 0;
			Buffer[1] = 30;
			if (optionum != 0x00)
				sprintf_s(Buffer + 8, 24, "%s", optionum);
			gets_s(Buffer + 32, 256);
			strcpy_s((lpr.msg->lastuser + 32), 256, (Buffer + 32));
			strcpy_s((lpr.msg->lastuser + 8), 256, (Buffer + 8));
			send(out, Buffer, 256, 0);
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

void CloseChat()
{
	closesocket(rcv);
	closesocket(out);
	WSACleanup();
	DeleteCriticalSection(&wrcon);
}
