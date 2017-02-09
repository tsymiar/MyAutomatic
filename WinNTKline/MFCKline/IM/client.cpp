#pragma comment(lib, "WS2_32.lib")
#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <conio.h>

using namespace std;
#define DEFAULT_PORT 8877
SOCKET in, out;
int loggedon = 0;
char lastmsg[256], lastgroup[256];
CRITICAL_SECTION wrcon;
void r(void) {
	char buf[256];
	int feedback;
	do {
		feedback = recv(in, buf, 256, 0);
		if (!(feedback == 256)) {
			Sleep(100);
			printf("connection lost\n");
			exit(0);
		};
		EnterCriticalSection(&wrcon);
		if (buf[1] == 31) {
			printf("received from %s:%s\n", (buf + 8), (buf + 32));
		}
		else if (buf[1] == 20) {
			printf("%s\n", (buf + 8));
		}
		else if (buf[1] == 21) {
			printf("%s\n", (buf + 32));
		}
		else if (buf[1] == 22) {
			printf("info of %s\n %s \n", (buf + 8), (buf + 32));
		}
		else if (buf[1] == 123) {
			printf("password changed successfully\n");
		}
		else if (buf[1] == 10) {
			printf("join to gruop %s successfully\n", (lastgroup + 8));
		}
		else if (buf[1] == 11) {
			printf("join to gruop %s rejected :%s\n", (lastgroup + 8), (buf + 32));
		}
		else if (buf[1] == 12) {
			printf("create gruop %s successfully\n", (lastgroup + 8));
		}
		else if (buf[1] == 13) {
			printf("create gruop %s rejected\n", (lastgroup + 8));
		}
		else if (buf[1] == 14) {
			printf("leave gruop %s successfully\n", (lastgroup + 8));
		}
		else if (buf[1] == 30) {
			printf("you said to %s : %s\n", (lastmsg + 8), (lastmsg + 32));
		}
		else if (buf[1] == 32) {
			printf("talk to %s failed\n", (lastmsg + 8));
		}
		else if (buf[1] == 122) {
		}


		else printf("other info %d\n", buf[1]);
		LeaveCriticalSection(&wrcon);

	} while (1);
};
int main(int argc, char *argv[]) {
	WSADATA wsaData;
	char optionchar, optionstr[24], address[20];
	char Buffer[256];
	int err = WSAStartup(0x202, &wsaData);
	if (err == SOCKET_ERROR) {
		cerr << "WSAStartup failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	InitializeCriticalSection(&wrcon);
	SetConsoleTitle("chat client");
	if (argc == 2) {
		strcpy(address, argv[1]);
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
		cerr << "connect() failed:error " << WSAGetLastError() << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}
	in = socket(AF_INET, SOCK_STREAM, 0);
	if (out == INVALID_SOCKET) {
		cerr << "socket() failed with error " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	if (connect(in, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
		cerr << "connect() failed:error " << WSAGetLastError() << WSAECONNREFUSED << endl;
		WSACleanup();
		return -1;
	}

	do {
		printf("connected to server,logon or reg a new account? (l/r):");
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
			recv(in, Buffer, 256, 0);
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
			recv(in, Buffer, 256, 0);
			if (Buffer[1] == 0)
				printf("registered succesfully\n");
			else
				printf("register failed\n");
		}
		else
			printf("please in put a valid option\n");
	} while (loggedon == 0);
	printf("help command to see help message\n");
	DWORD thread_ID;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(r), NULL, 0, &thread_ID);
	char onechar;
	char auxstr[24];
	do {
		fflush(stdin);
		onechar = getch();
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
			MessageBox(NULL, "quit to quit program\nhelp to show this message\nlist to see online user list\nallgroup to see group list on this server\nmemberof groupname to see user list of this group\nsetinfo newinfo to set personal info\ninfo name  to see introduction of a user\ncreategroup groupname grouppassword to create a new group\njoingroup groupname password to join a group\nquitgroup groupname to leave a group\nuser/groupname message to talk to a user or group\npassword newpasswrd to set password\n", "help message", MB_OK);
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
			strcpy((lastgroup + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "joingroup") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 10;
			scanf("%s%s", (Buffer + 8), (Buffer + 32));
			strcpy((lastgroup + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else if (strcmp(optionstr, "quitgroup") == 0) {
			Buffer[0] = 0;
			Buffer[1] = 12;
			scanf("%s", (Buffer + 8));
			strcpy((lastgroup + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		}
		else {
			Buffer[0] = 0;
			Buffer[1] = 30;
			strcpy((Buffer + 8), optionstr);
			gets_s(Buffer + 32, 256);
			strcpy((lastmsg + 32), (Buffer + 32));
			strcpy((lastmsg + 8), (Buffer + 8));
			send(out, Buffer, 256, 0);
		};
		LeaveCriticalSection(&wrcon);
	} while (loggedon == 1);
	printf("quit now\n");
	closesocket(in);
	closesocket(out);
	WSACleanup();
	DeleteCriticalSection(&wrcon);
	return 0;
}
