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
	char address[20];
	int err = WSAStartup(0x202, &wsaData);
	if (err == SOCKET_ERROR) {
		cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	InitializeCriticalSection(&wrcon);
	SetConsoleTitle("chat client");
	if (argc == 2) {
		strcpy(address, argv/*[1]*/);
	}
	else {
		printf("enter server address:");
		scanf("%s", &address);
	};
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(address);
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

int StartChat(int err, void(*func)(void*))
{
	if (err != 0)
		return err;
	char onechar;
	struct LPR lpr;
	DWORD thread_ID;
	char auxstr[24];
	char Buffer[256];
	char optionchar, optionstr[24];

	do {
		printf("Connect to server OK, [logon] or [regist] a new account? (l/r):");
		fflush(stdin);
		optionchar = getchar();
		if (optionchar == 'l') {
			Buffer[0] = 0;
			Buffer[1] = (char)120;
			printf("user name:");
			scanf("%s", (Buffer + 8));
			char title[30];
			strcpy(title, "chat client,logged on as ");
			strcat(title, (Buffer + 8));
			printf("password:");
			scanf("%s", (Buffer + 32));
			send(out, Buffer, 256, 0);
			recv(rcv, Buffer, 256, 0);
			if (Buffer[1] == 120) {
				printf("logged on successfully\n");
				SetConsoleTitle(title);
				loggedon = 1;
			}
			else
				printf("log on failed\n");
		}
		else if (optionchar == 'r') {
			Buffer[0] = 0;
			Buffer[1] = 0;
			printf("name want to use:");
			scanf("%s", (Buffer + 8));
			printf("password:");
			scanf("%s", (Buffer + 32));
			send(out, Buffer, 256, 0);
			recv(rcv, Buffer, 256, 0);
			if (Buffer[1] == 0)
				printf("registered succesfully\n");
			else
				printf("register failed\n");
		}
		else
			printf("please in put a valid option\n");
	} while (loggedon == 0);
	printf("type command [help] to see help message\n");
	lpr.sock = rcv;
	lpr.wrcon = wrcon;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(func), &lpr, 0, &thread_ID);
	do {
		fflush(stdin);
		onechar = _getch();
		EnterCriticalSection(&wrcon);

		auxstr[0] = onechar;
		auxstr[1] = 0;
		fflush(stdin);
		scanf("%s", optionstr);
		// strcat(auxstr,optionstr);
		// strcpy(optionstr,auxstr);
		if (strcmp(optionstr, "quit") == 0) {
			Buffer[0] = 0;
			Buffer[1] = (char)121;
			send(out, Buffer, 256, 0);
			loggedon = 0;
		}
		else if (strcmp(optionstr, "help") == 0) {/*
												  printf("\nyes,let me help you\n");
												  printf("quit to quit program\nhelp to show this message\nlist to see online user list\nallgroup to see group list on this server\n");
												  printf("memberof groupname to see user list of this group\nsetinfo newinfo to set personal info\ninfo name  to see introduction of a user\ncreategroup groupname grouppassword to create a new group\n");
												  printf("joingroup groupname password to join a group\nquitgroup groupname to leave a group\n");
												  printf("user/groupname message to talk to a user or group\npassword newpasswrd to set password\n\n");
												  */
			MessageBox(NULL, "[quit]\nto quit program\n[help]\nto show this message\n[list]\nto see online user list\n[allgroup]\nto see group list on this server\n[memberof groupname]\nto see user list of this group\n[setinfo newinfo]\nto set personal info\n[info name]\nto see introduction of a user\n[creategroup groupname grouppassword]\nto create a new group\n[joingroup groupname password]\nto join a group\n[quitgroup groupname]\nto leave a group\n[user/groupname message]\nto talk to a user or group\n[password newpasswrd]\nto set password\n", "help message", MB_OK);
		}
		else if (strcmp(optionstr, "list") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 20;
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "allgroup") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 15;
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "memberof") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 16;
			scanf("%s", (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "setinfo") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 1;
			gets_s(Buffer + 32, 256);
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "info") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 21;
			scanf("%s", (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "password") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 122;
			scanf("%s", (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "creategroup") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 11;
			scanf("%s%s", (Buffer + 8), (Buffer + 32));
			strcpy((lpr.msg->lastgroup + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "joingroup") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 10;
			scanf("%s%s", (Buffer + 8), (Buffer + 32));
			strcpy((lpr.msg->lastgroup + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "quitgroup") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 12;
			scanf("%s", (Buffer + 8));
			strcpy((lpr.msg->lastgroup + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else {
			Buffer[0] = 0;
			Buffer[1] = 30;
			strcpy((Buffer + 8), optionstr);
			gets_s(Buffer + 32, 256);
			strcpy((lpr.msg->lastuser + 32), (Buffer + 32));
			strcpy((lpr.msg->lastuser + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		};
		LeaveCriticalSection(&wrcon);
	} while (loggedon == 1);
	printf("quit now\n");
	return 0;
}

void CloseChat()
{
	closesocket(rcv);
	closesocket(out);
	WSACleanup();
	DeleteCriticalSection(&wrcon);
}