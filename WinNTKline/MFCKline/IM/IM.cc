#include <stdio.h>
#include <iostream>
#include <fstream>
#ifdef WIN32
#pragma comment(lib, "WS2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>  
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#define DEFAULT_PORT 8877
#define MAX_USERS 200
#define MAX_ONLINE 100
#define MAX_MENBERS_PER_GROUP 20
#define MAX_GROUPS 20
#define ACC_REC "accounts.bin"
#define DEBUGINFO 0
char Buffer[256];
int  havemsg, wantquit, c0, c1;
char tosend[256];
#ifdef WIN32
WSADATA wsaData;
SOCKET
#else
int
#endif
recvtemp, sendtemp, listen_socket;
#ifdef __linux
pthread_mutexattr_t attr;
#endif
#ifdef WIN32
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
sendallow;
struct user {
	char name[24];
	char passwd[24];
	char intro[224];
};
user users[MAX_USERS];
struct group {
	char groupname[24];
	char passwd[24];
	char member[MAX_MENBERS_PER_GROUP][24];
};
group groups[MAX_GROUPS];
struct LINE {
	char name[24];
#ifdef WIN32
	SOCKET
#else
	int
#endif
		recvsocket;
#ifdef WIN32
	SOCKET
#else
	int
#endif
		sendsocket;
} online[MAX_ONLINE];
void dump(void) {
#ifdef WIN32
	_flushall();
#else
	fflush(stdin);
#endif
	FILE *dumpfile;
	dumpfile = fopen(ACC_REC, "w");
	fwrite(users, sizeof(user), MAX_USERS, dumpfile);
	fwrite(groups, sizeof(group), MAX_GROUPS, dumpfile);
	fclose(dumpfile);
#ifdef WIN32
	_flushall();
#else
	fflush(stdin);
#endif
};
int resume(void) {   //load file system from disk
	FILE *dumpfile;
#ifdef WIN32
	_flushall();
#else
	fflush(stdin);
#endif
	dumpfile = fopen(ACC_REC, "r");
	if (dumpfile == NULL)
		return -1;
	else {
		fread(users, sizeof(user), MAX_USERS, dumpfile);
		fread(groups, sizeof(group), MAX_GROUPS, dumpfile);
		fclose(dumpfile);
		return 0;
	};
#ifdef WIN32
	_flushall();
#else
	fflush(stdin);
#endif
};
int user_to_sock(char user_name[24]) {
	int c;
	char tomatch[24];
	strcpy(tomatch, user_name);
	for (c = 0; c<MAX_ONLINE; c++) {
		if (strcmp(tomatch, online[c].name) == 0)
			return c;
	}
	return -1;
};
int adduser(char name[24], char passwd[24]) {
	int c;
	char n[24], p[24];
	strcpy(n, name);
	strcpy(p, passwd);
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].name) == 0)
			return c;
	};
	for (c = 0; c<MAX_USERS; c++) {
		if (strlen(users[c].name) == 0) {
			strcpy(users[c].name, n);
			strcpy(users[c].passwd, p);
			strcpy(users[c].intro, "info not set");
			return -1;
		};
	};
	return -2;
};
int getusernum(char name[24]) {
	int c;
	char n[24];
	strcpy(n, name);
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].name) == 0)
			return c;
	};
	return -1;
};
int auth(char name[24], char passwd[24]) {
	int c;
	char n[24], p[24];
	strcpy(n, name);
	strcpy(p, passwd);
	for (c = 0; c<MAX_USERS; c++) {
		if ((strcmp(n, users[c].name) == 0) && (strcmp(p, users[c].passwd) == 0)) {
			if (user_to_sock(n) == -1)
				return 0;   //pass
			else
				return 1;   //already online
		}
	};
	return 2;      //wrong name or passwd
};
int addgroup(char groupname[24], char passwd[24]) {
	char n[24], p[24];
	int c;
	strcpy(n, groupname);
	strcpy(p, passwd);
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strlen(groups[c].groupname) == 0) {
			strcpy(groups[c].groupname, n);
			strcpy(groups[c].passwd, p);
			return c;
		};
	};
	return -1;
};
int findgroupnum(char groupname[24]) {
	char n[24];
	int c;
	strcpy(n, groupname);
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strcmp(groups[c].groupname, n) == 0)
			return c;
	};
	return -1;
};
int addtogroup(int groupnum, char name[24], char passwd[24]) {
	char n[24], p[24];
	int c;
	strcpy(n, name);
	strcpy(p, passwd);
	for (c = 0; c<MAX_MENBERS_PER_GROUP; c++) {
		if (strcmp(groups[groupnum].member[c], n) == 0)
			return -1;
	};
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strlen(groups[groupnum].member[c]) == 0)) {
			if (strcmp(groups[groupnum].passwd, p) == 0) {
				strcpy(groups[groupnum].member[c], n);
				return c;
			}
			else
				return -2;
		};
	};
	return -3;
};
int leavegroup(int groupnum, char name[24]) {
	char n[24];
	int c;
	strcpy(n, name);
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strcmp(groups[groupnum].member[c], n) == 0)) {
			strcpy(groups[groupnum].member[c], "");
			if (c == 0)
				strcpy(groups[groupnum].groupname, "");
			return 0;
		};
	};
	return -1;
};
#ifdef __linux
void *monite(void *arg)
#else
void monite(void)
#endif
{
#ifdef WIN32
	SOCKET
#else
	int
#endif
		recvsock = recvtemp,//½ÓÊÕ
		sendsock = sendtemp;//·¢ËÍ
	int c, feedback, loggedon = 0;
	int qq = 1;
	char buf[256], bufs[256], name[24];
	do {
		feedback = recv(recvsock, buf, 256, 0);
		if (feedback == -1) {
			goto con_err1;
		};
		if (DEBUGINFO)
			printf("rcvd %d,%d\n", buf[0], buf[1]);
		if ((buf[0] == 0) && (buf[1] == 0)) {
			bufs[0] = 1;
			feedback = adduser((buf + 8), (buf + 32));
			if (feedback == -1) {
				printf("new user: %s\n", (buf + 8));
				addtogroup(0, (buf + 8), (char*)"all");
				bufs[1] = 0;
			}
			else if (feedback == -2) {
				bufs[1] = 1;
				strcpy((buf + 32), "too many users");
			}
			else {
				bufs[1] = 1;
				strcpy((buf + 32), "user already exhists");
			};
			send(sendsock, bufs, 256, 0);
			if (DEBUGINFO)
				printf("%d and %d \n", bufs[0], bufs[1]);
		}
		else if ((buf[0] == 0) && (buf[1] == 120)) {
			bufs[0] = 1;
			strcpy(name, (buf + 8));
			feedback = auth((buf + 8), (buf + 32));
			if (feedback == 0) {
				printf("%s logged on\n", (buf + 8));
				bufs[1] = (char)120;
				loggedon = 1;
			}
			else if (feedback == 1) {
				bufs[1] = (char)121;
				strcpy((buf + 32), "already online");
			}
			else {
				bufs[1] = (char)121;
				strcpy((buf + 32), "wrong name or password");
			};
			send(sendsock, bufs, 256, 0);
		}
	} while (loggedon == 0);
	for (c = 0; c<MAX_ONLINE; c++) {
		if (strlen(online[c].name) == 0) {
			online[c].recvsocket = recvsock;
			online[c].sendsocket = sendsock;
			strcpy(online[c].name, buf + 8);
			break;
		};
	};
	do {
		feedback = recv(recvsock, buf, 256, 0);
		if (!(feedback == 256)) {
			printf("connection with %s lost\n", name);
			goto con_err;
		};
		if (DEBUGINFO)
			printf("rcvd %d,%d\n", buf[0], buf[1]);
		bufs[0] = 1;
		if ((buf[0] == 0) && (buf[1] == (char)121)) {
			bufs[1] = (char)122;
			loggedon = 0;
			send(sendsock, bufs, 256, 0);
			printf("%s quit \n", name);
		}
		else if ((buf[0] == 0) && (buf[1] == (char)1)) {
			feedback = getusernum(name);
			strcpy(users[feedback].intro, (buf + 32));
		}
		else if ((buf[0] == 0) && (buf[1] == 122)) {
			feedback = getusernum(name);
			strcpy(users[feedback].passwd, (buf + 8));
			bufs[1] = (char)123;
			send(sendsock, bufs, 256, 0);
		}

		else if ((buf[0] == 0) && (buf[1] == 20)) {
			bufs[1] = (char)21;
			strcpy((bufs + 32), "onlien user list:");
			send(sendsock, bufs, 256, 0);
			for (c = 0; c<MAX_ONLINE; c++) {
				if (strlen(online[c].name) >> 0) {
					bufs[1] = (char)20;
					strcpy((bufs + 8), online[c].name);
					send(sendsock, bufs, 256, 0);
				};
			};
		}

		else if ((buf[0] == 0) && (buf[1] == 15)) {
			bufs[1] = (char)21;
			strcpy((bufs + 32), "groups list:");
			send(sendsock, bufs, 256, 0);
			for (c = 0; c<MAX_GROUPS; c++) {
				if (strlen(groups[c].groupname) >> 0) {
					bufs[1] = (char)20;
					strcpy((bufs + 8), groups[c].groupname);
					send(sendsock, bufs, 256, 0);
				};
			};
		}

		else if ((buf[0] == 0) && (buf[1] == 16)) {
			bufs[1] = (char)21;
			feedback = findgroupnum(buf + 8);
			if (feedback == -1) {
				strcpy((bufs + 32), "no such group");
				send(sendsock, bufs, 256, 0);
			}
			else {
				strcpy((bufs + 32), "users of this group:");
				send(sendsock, bufs, 256, 0);
				bufs[1] = 20;
				for (c = 0; c<MAX_MENBERS_PER_GROUP; c++) {
					if (strlen(groups[feedback].member[c]) >> 0) {
						strcpy((bufs + 8), groups[feedback].member[c]);
						send(sendsock, bufs, 256, 0);
					};
				};
			};
		}
		else if ((buf[0] == 0) && (buf[1] == 11)) {
			feedback = addgroup((buf + 8), (buf + 32));
			if (feedback == -1) {
				strcpy((bufs + 32), "rejected");
				bufs[1] = 13;
			}
			else {
				printf("new group: %s\n", (buf + 8));
				addtogroup(feedback, name, (buf + 32));
				bufs[1] = 12;
			}
			send(sendsock, bufs, 256, 0);
		}

		else if ((buf[0] == 0) && (buf[1] == 10)) {
			feedback = addtogroup(findgroupnum(buf + 8), name, (buf + 32));
			if (feedback == -1) {
				strcpy((bufs + 32), "you have already in this group");
				bufs[1] = 11;
			}
			else if (feedback == -2) {
				strcpy((bufs + 32), "wrong password to this group");
				bufs[1] = 11;
			}
			else {
				bufs[1] = 10;
			};
			send(sendsock, bufs, 256, 0);
		}

		else if ((buf[0] == 0) && (buf[1] == 12)) {
			feedback = leavegroup(findgroupnum(buf + 8), name);
			if (feedback == 0)
				strcpy((bufs + 32), "leave group successfully");
			else
				strcpy((bufs + 32), "you are not in this group");
			bufs[1] = 14;
			send(sendsock, bufs, 256, 0);
		}
		else if ((buf[0] == 0) && (buf[1] == 21)) {
			feedback = getusernum(buf + 8);
			if (feedback >= 0) {
				strcpy((bufs + 8), (buf + 8));
				strcpy((bufs + 32), users[feedback].intro);
			}
			else {
				strcpy((bufs + 8), "system info");
				strcpy((bufs + 32), "no such user");
			}
			bufs[1] = 22;
			send(sendsock, bufs, 256, 0);
		}
		else if ((buf[0] == 0) && (buf[1] == 30)) { //loop1
			feedback = findgroupnum(buf + 8);
			if (feedback == -1) {//feedback=-1
				feedback = user_to_sock(buf + 8);
				if (feedback == -1) {
					bufs[1] = 32;
				}
				else {          //loop0
					bufs[1] = 31;
					strcpy((bufs + 32), (buf + 32));
					strcpy((bufs + 8), name);
					send(online[feedback].sendsocket, bufs, 256, 0);
					bufs[1] = 30;
				};    //loop0

				send(sendsock, bufs, 256, 0);
			}//feedback=-1
			else {
				for (c = 0; c<MAX_MENBERS_PER_GROUP; c++) {
					if (!strlen(groups[feedback].member[c]) == 0)
					{
						bufs[1] = 31;
						int sss = user_to_sock(groups[feedback].member[c]);
						if (!(sss == -1)) {
							strcpy((bufs + 32), (buf + 32));
							strcpy((bufs + 8), name);
							send(online[sss].sendsocket, bufs, 256, 0);
						}//if sss
					}//if strlen
				}//for
				bufs[1] = 30;
				send(sendsock, bufs, 256, 0);
			}//else
		};//else if
	} while (loggedon == 1);
con_err:
	feedback = user_to_sock(name);
	strcpy(online[feedback].name, "");

con_err1:
#ifdef WIN32
	closesocket(recvsock);
	closesocket(sendsock);
#else
	close(recvsock);
	close(sendsock);
#endif
	return NULL;
};

#ifdef __linux
void *control(void *arg)
#else
void control(void)
#endif
{
	char optionstr[24], name[24];
	int feedback;
	do {
		scanf("%s", &optionstr);
		if (strcmp(optionstr, "quit") == 0) {
#ifdef WIN32
			closesocket(listen_socket);
#else
			close(listen_socket);
#endif
			printf("saving accounts data to file %s\n", ACC_REC);
			dump();
#ifdef WIN32
			WSACleanup();
			DeleteCriticalSection(&sendallow);
#else
			pthread_mutex_destroy(&sendallow);
#endif
			wantquit = 1;
			exit(0);
		}
		if (strcmp(optionstr, "kick") == 0) {
			scanf("%s", &name);
			feedback = user_to_sock(name);
			if (!(feedback == -1)) {
#ifdef WIN32
				closesocket(online[feedback].recvsocket);
				closesocket(online[feedback].sendsocket);
#else
				close(online[feedback].recvsocket);
				close(online[feedback].sendsocket);
#endif
				printf("user %s kicked !\n", name);
			};
		};
	} while (!(wantquit));
};

int main(int argc, char *argv[]) {

	if (!(resume()))
		printf("accounts data loaded from %s\n", ACC_REC);
	else {
		strcpy(users[0].name, "admin");
		strcpy(users[0].passwd, "admin");
		strcpy(groups[0].groupname, "all");
		strcpy(groups[0].passwd, "all");
		strcpy(groups[0].member[0], "admin");
	}
#ifdef WIN32
	InitializeCriticalSection(&sendallow);
#else
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&(sendallow), &attr);
#endif
	wantquit = 0;
#ifdef WIN32
	SetConsoleTitle((LPCWSTR)"chat server for network design");
	DWORD
#else
	pthread_t
#endif
		thread_ID;
#ifdef __linux
	pthread_create(&thread_ID, NULL, control, (void*)-1);
#else
#ifdef WIN32
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(control), NULL, 0, &thread_ID);
#endif
#endif
	int err =
#ifdef WIN32
		WSAStartup(0x202, &wsaData);
#else
		0;
#endif
	if (err
#ifdef WIN32
		== SOCKET_ERROR) {
		//cerr<<"WSAStartup failed with error "<<WSAGetLastError()<<endl;
		WSACleanup();
#else
		< 0) {
#endif
		return -1;
	}
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(DEFAULT_PORT);
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket
#ifdef WIN32
		== INVALID_SOCKET) {
		//cerr<<"socket() failed with error "<<WSAGetLastError()<<endl;
		WSACleanup();
#else
		< 0) {
#endif
		return -1;
	}
	if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local))
#ifdef WIN32
		== SOCKET_ERROR) {
		//cerr<<"bind() failed with error "<<WSAGetLastError()<<endl;
		WSACleanup();
#else
		< 0) {
#endif
		return -1;
	}
	if (listen(listen_socket, 50)
#ifdef WIN32
		== SOCKET_ERROR) {
		//cerr<<"listen() failed with error "<<WSAGetLastError()<<endl;
		WSACleanup();
#else
		< 0) {
#endif
		return -1;
	}
	printf("listening to port %d\n", DEFAULT_PORT);
	struct sockaddr_in from;
	socklen_t fromlen = (socklen_t)sizeof(from);
	int c = 0;
	do {
		recvtemp = accept(listen_socket, (struct sockaddr*)&from, &fromlen);

		if (recvtemp
#ifdef WIN32
			== INVALID_SOCKET) {
			//cerr<<"accept() failed error "<<WSAGetLastError();
			WSACleanup();
#else
			< 0) {
#endif
			return -1;
		};
		if (DEBUGINFO)
			printf("1:%d\n", recvtemp);
		sendtemp = accept(listen_socket, (struct sockaddr*)&from, &fromlen);

		if (sendtemp
#ifdef WIN32
			== INVALID_SOCKET) {
			//cerr<<"accept() failed error "<<WSAGetLastError();
			WSACleanup();
#else
			< 0) {
#endif
			return -1;
			return -1;
		};
#ifdef __linux
		pthread_create(&thread_ID, NULL, monite, (void*)-1);
#else
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(monite), NULL, 0, &thread_ID);
#endif
		c++;
		if (DEBUGINFO)
			printf("2:%d\n", sendtemp);
		if (DEBUGINFO)
			if (DEBUGINFO)
				printf("c is : %d", c);
		} while (!(wantquit));
		return 0;
}
