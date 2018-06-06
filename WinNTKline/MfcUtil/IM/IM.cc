#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#ifdef _WIN32
#pragma comment(lib, "WS2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#else
#include <netinet/in.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>  
#endif
#define DEFAULT_PORT 8877
#define MAX_USERS 300
#define MAX_ONLINE 100
#define MAX_MENBERS_PER_GROUP 20
#define MAX_GROUPS 20
#define ACC_REC "acnts"
//using namespace std;
//C++11 std与socket.h中bind函数冲突
#ifdef _WIN32
typedef int type_len;
typedef SOCKET type_socket;
#ifndef pthread_t
typedef unsigned int pthread_t;
#endif
typedef unsigned int type_thread_func;
#define flush_all() _flushall()
#else
typedef int type_socket;
typedef socklen_t type_len; 
typedef void *type_thread_func;
#define flush_all() fflush(stdin)
#define closesocket(socket) close(socket)
pthread_mutexattr_t attr;
#endif

#ifdef _WIN32
WSADATA wsaData;
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
sendallow;

type_socket listen_socket, recv_socket;
int  aimtoquit;

struct msg {
	char _0_ = '\0';
	char idx;
	char ret[2];
	char rsv[4];
	char usr[24];
	char key[24];
};

struct user {
	char name[24];
	char psw[24];
	char intro[224];
}users[MAX_USERS];

struct group {
	char ngrp[24];
	char psw[24];
	char members[MAX_MENBERS_PER_GROUP][24];
}groups[MAX_GROUPS];

struct LINE {
	char user[24];
	type_socket sock_rcv;
	type_socket sock_snd;
} online[MAX_ONLINE];


int _im_(int argc, char *argv[]);

int main(
#ifdef __linux
	int argc, char* argv[]
#endif
)
{
#ifdef __linux
	if (fork() == 0)
#else
	int argc = 1;
	char* argv[] = { NULL };
#endif
	_im_(argc, argv);
	return 0;
}

void save_acnt();
int load_acnt();
int new_user(char usr[24], char psw[24]);
int user_to_line(char user[24]);
int get_user_idx(char user[24]);
int user_auth(char usr[24], char psw[24]);
int host_group(char ngrp[24], char psw[24]);
int get_group_idx(char group[24]);
int join_group(int idx, char usr[24], char psw[24]);
int leave_group(int idx, char usr[24]);

type_thread_func monite(void *arg)
{
	int c, flg, rtn;
	int qq = 0, logged = 0;
	char name[24], bufs[256], tmp[256];
	struct sockaddr_in adin;
	type_len len = (type_len)sizeof(adin);
	type_socket rcv_sock = recv_socket,//接收
		snd_sock = //发送
		accept(listen_socket, (struct sockaddr*)&adin, &len);

	if (snd_sock
#ifdef _WIN32
		== INVALID_SOCKET) {
		std::cerr << "accept() failed error " << WSAGetLastError();
		WSACleanup();
#else
		< 0) {
#endif
		std::cerr << "errno:\t" << strerror(errno) << std::endl;
		return -1;
	};
#ifdef _DEBUG
	printf("2: %d\n", snd_sock);
#endif
	do {
		memset(name, 0, sizeof(name));
		memset(bufs, 0, 256);
		memset(tmp, 0, 256);
		flg = recv(rcv_sock, tmp, 256, 0);
		if (flg == -1) {
			goto con_err1;
		};
#ifdef _DEBUG
		if (qq != flg && flg != 0)
		{
			printf("1rcv[%0x,%0x]:%s\n", tmp[0], tmp[1], tmp + 8);
			for (c = 0; c < 256; c++)
			{
				if (c % 32 == 0)
					printf("\n");
				printf("%02x ", (unsigned)tmp[c]);
			}
			printf("\n");
		}
#endif
		// tmp: inc--8 bit crc_head, 24 bit username, 24 bit password.
		if ((tmp[0] == 0) && (tmp[1] == 0)) {
			bufs[0] = bufs[1] = 0;
			rtn = new_user((tmp + 8), (tmp + 32));
			if (rtn == -1) {
				sprintf(bufs + 2, "%x", -1);
				sprintf(bufs + 8, "new user: %s\n", (tmp + 8));
				join_group(0, (tmp + 8), (char*)"all");
			}
			else if (rtn == -2) {
				sprintf(bufs + 2, "%x", -2);
				strcpy((bufs + 8), "too many users.\n");
			}
			else if (rtn > 0) {
				sprintf(bufs + 2, "%x", -3);
				strcpy((bufs + 8), "user already exists.\n");
			};
			send(snd_sock, bufs, strlen(bufs) + 1, 0);
#ifdef _DEBUG
			if (qq != flg && flg != 0)
				printf("bufs[%0x,%0x]:%s\n", bufs[0], bufs[1], bufs + 8);
#endif
		}
		else if ((tmp[0] == 0) && (tmp[1] == 0x1)) {
			bufs[0] = 0;
			bufs[1] = 0x1;
			sprintf(bufs + 2, "%x", (rtn = user_auth((tmp + 8), (tmp + 32))));
			strcpy(name, (tmp + 8));
			if (rtn == 0) {
				logged = 1;
				sprintf(bufs + 8, "[%s] logged on.\n", (tmp + 8));
			}
			else if (rtn == 1) {
				sprintf(bufs + 1, "%x", 1);
				strcpy((tmp + 8), "already online.\n");
			}
			else {
				strcpy((tmp + 8), "check username/password again.\n");
			};
			send(snd_sock, bufs, 256, 0);
		}
		// set cur qq as last flg;
		qq = flg;
		for (c = 0; c<MAX_ONLINE; c++) {
			if (strlen(online[c].user) == 0) {
				online[c].sock_rcv = rcv_sock;
				online[c].sock_snd = snd_sock;
				strcpy(online[c].user, tmp + 8);
				break;
			};
		};
		while (logged) {
			flg = recv(rcv_sock, tmp, 256, 0);
			if (flg < 256)
				goto con_err;
			if (flg == -1) {
				printf("lost connection with %s.\n", name);
				goto con_err1;
			};
#ifdef _DEBUG
			if (qq != flg && flg != 0)
				printf("2rcv[%x,%x]:%s\n", tmp[0], tmp[1], tmp + 8);
			qq = flg;
#endif
			memcpy(bufs, tmp, 2);
			if (tmp[0] == 0)
			{
				switch (tmp[1])
				{
				case 0x3:
				{
					bufs[2] = 0x0;
					logged = 0;
					sprintf(bufs + 8, "%s quit.\n", name);
					send(snd_sock, bufs, 256, 0);
				} return 0;
				case 0x4:
				{
					bufs[2] = 0x0;
					rtn = get_user_idx(name);
					strcpy(users[rtn].intro, (tmp + 32));
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0x5:
				{
					bufs[2] = 0x0;
					rtn = get_user_idx(name);
					strcpy(users[rtn].psw, (tmp + 32));
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0x6:
				{
					bufs[2] = 0x0;
					strcpy((bufs + 8), "users online list:");
					send(snd_sock, bufs, 256, 0);
					for (c = 0; c < MAX_ONLINE; c++) {
						if (strlen(online[c].user) >> 0) {
							sprintf(bufs + 2, "%x", c);
							strcpy((bufs + 8 * (c + 1)), online[c].user);
							send(snd_sock, bufs, 256, 0);
						};
					};
				} break;
				case 0x7:
				{
					bufs[2] = 0x0;
					strcpy((bufs + 8), "groups list:");
					send(snd_sock, bufs, 256, 0);
					for (c = 0; c < MAX_GROUPS; c++) {
						if (strlen(groups[c].ngrp) >> 0) {
							bufs[1] = c;
							strcpy((bufs + 8 * (c + 1)), groups[c].ngrp);
							send(snd_sock, bufs, 256, 0);
						};
					};
				} break;
				case 0x8:
				{
					rtn = get_group_idx(tmp + 8);
					sprintf(bufs, "%x", rtn);
					if (rtn == -1) {
						strcpy((bufs + 8), "no such group.\n");
					}
					else {
						strcpy((bufs + 8), "users of this group:");
						for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
							if (strlen(groups[rtn].members[c]) >> 0) {
								strcpy((bufs + 8 * (c + 1)), groups[rtn].members[c]);
							};
						};
					};
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0x9:
				{
					rtn = host_group((tmp + 8), (tmp + 32));
					sprintf(bufs, "%x", rtn);
					if (rtn == -1) {
						strcpy((bufs + 8), "rejected");
					}
					else {
						join_group(rtn, name, (tmp + 32));
						sprintf(bufs + 8, "new group: %s.\n", (tmp + 8));
					}
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0xA:
				{
					rtn = join_group(get_group_idx(tmp + 8), name, (tmp + 32));
					sprintf(bufs, "%x", rtn);
					if (rtn == -1) {
						strcpy((bufs + 8), "you have already in this group.\n");
					}
					else if (rtn == -2) {
						strcpy((bufs + 8), "wrong password to this group.\n");
					}
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0xB:
				{
					rtn = leave_group(get_group_idx(tmp + 8), name);
					sprintf(bufs, "%x", rtn);
					if (rtn == 0)
						strcpy((bufs + 8), "leave group successfully.\n");
					else
						strcpy((bufs + 8), "you are not in this group.\n");
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0xC:
				{
					rtn = get_user_idx(tmp + 8);
					sprintf(bufs, "%x", rtn);
					if (rtn >= 0) {
						strcpy((bufs + 8), (tmp + 8));
						strcpy((bufs + 32), users[rtn].intro);
					}
					else {
						strcpy((bufs + 8), "system info:\nNo such user!\n");
					}
					send(snd_sock, bufs, 256, 0);
				} break;
				case 0xD:
				{ //loop1
					rtn = get_group_idx(tmp + 8);
					sprintf(bufs, "%x", rtn);
					if (rtn == -1) {//rtn=-1
						if (-1 != user_to_line(tmp + 8)) {
							sprintf(bufs, "%x", -1);
							strcpy((bufs + 8), name);
							strcpy((bufs + 32), (tmp + 32));
							send(online[rtn].sock_snd, bufs, 256, 0);
						};    //loop0
					}//rtn=-1
					else {
						for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
							if (!strlen(groups[rtn].members[c]) == 0)
							{
								sprintf(bufs, "%x", -3);
								int sss = user_to_line(groups[rtn].members[c]);
								if (!(sss == -1)) {
									strcpy((bufs + 8), name);
									strcpy((bufs + 32), (tmp + 32));
									send(online[sss].sock_snd, bufs, 256, 0);
								}//if sss
							}//if strlen
						}//for
						send(snd_sock, bufs, 256, 0);
					}//else
				} break;
				default: break;
				}
			}
		};
	} while (!logged);
	
con_err:
	rtn = user_to_line(name);
	strcpy(online[rtn].user, "");

con_err1:
	closesocket(rcv_sock);
	closesocket(snd_sock);

	return NULL;
};

type_thread_func commands(void *arg)
{
	char optionstr[24], name[24];
	int rtn;
	do {
		scanf("%s", (char*)&optionstr);
		if (strcmp(optionstr, "quit") == 0) {
			closesocket(listen_socket);
			printf("saving accounts data to file %s.\n", ACC_REC);
			save_acnt();
#ifdef _WIN32
			WSACleanup();
			DeleteCriticalSection(&sendallow);
#else
			pthread_mutex_destroy(&sendallow);
#endif
			aimtoquit = 1;
			exit(0);
		}
		if (strcmp(optionstr, "kick") == 0) {
			scanf("%s", (char*)&name);
			rtn = user_to_line(name);
			if (!(rtn == -1)) {
				closesocket(online[rtn].sock_rcv);
				closesocket(online[rtn].sock_snd);
				printf("user %s kicked !\n", name);
			};
		};
	} while (!(aimtoquit));
	return NULL;
};

int _im_(int argc, char *argv[]) {

	if (!(load_acnt()))
		printf("accounts data loaded from [%s].\n", ACC_REC);
	else {
		strcpy(users[0].name, "admin");
		strcpy(users[0].psw, "admin");
		strcpy(groups[0].ngrp, "all");
		strcpy(groups[0].psw, "all");
		strcpy(groups[0].members[0], "admin");
	}
#ifdef _WIN32
	InitializeCriticalSection(&sendallow);
#else
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&(sendallow), &attr);
#endif
	aimtoquit = 0;
#ifdef _WIN32
	SetConsoleTitle((
#ifdef _UNICODE
		LPCWSTR
#else
		LPCSTR
#endif
		)"chat server for network design");
#endif
	pthread_t thread_ID;
#ifdef _WIN32
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)commands, NULL, 0, &thread_ID);
#else
	pthread_create(&thread_ID, NULL, commands, (void*)-1);
#endif
	int err = 0;
#ifdef _WIN32
	err = WSAStartup(0x202, &wsaData);
#endif
	if (err
#ifdef _WIN32
		== SOCKET_ERROR) {
		std::cerr << "WSAStartup failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
#else
		< 0) {
#endif
		std::cerr << "errno:\t" << strerror(errno) << std::endl;
		return -1;
	}
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(DEFAULT_PORT);
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket
#ifdef _WIN32
		== INVALID_SOCKET) {
		std::cerr << "socket() failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
#else
		< 0) {
#endif
		return -1;
	}
	if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local))
#ifdef _WIN32
		== SOCKET_ERROR) {
		std::cerr << "bind() failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
#else
		< 0) {
#endif
		std::cerr << "errno:\t" << strerror(errno) << std::endl;
		return -1;
	}
	if (listen(listen_socket, 50)
#ifdef _WIN32
		== SOCKET_ERROR) {
		std::cerr << "listen() failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
#else
		< 0) {
#endif
		std::cerr << "errno:\t" << strerror(errno) << std::endl;
		return -1;
	}
	printf("listening to port %d.\n", DEFAULT_PORT);
	struct sockaddr_in from;
	type_len fromlen = (type_len)sizeof(from);
	int c = 0;
	char IPdotdec[16];
	do {
		recv_socket = accept(listen_socket, (struct sockaddr*)&from, &fromlen);
		if (recv_socket
#ifdef _WIN32
			== INVALID_SOCKET) {
			std::cerr << "accept() failed error " << WSAGetLastError();
			WSACleanup();
#else
			< 0) {
#endif
			std::cerr << "errno:\t" << strerror(errno) << std::endl;
			return -1;
		}
		else
		{
#ifdef __linux
			inet_ntop(AF_INET, (void *)&from.sin_addr, IPdotdec, 16);
#else
			memcpy(IPdotdec, inet_ntoa(from.sin_addr), 16);
#endif
			printf("accept() [%s] OK.\n", IPdotdec);
		}
#ifdef _DEBUG
		printf("1: %d\n", recv_socket);
#endif
#ifdef _WIN32
		_beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, NULL, 0, &thread_ID);
#else
		pthread_create(&thread_ID, NULL, monite, (void*)-1);
#endif
		c++;
		} while (!(aimtoquit));
		printf("c = %d\n", c);
		return 0;
		}
//save accounts to file.
void save_acnt() {
	flush_all();
	FILE *dumpfile;
	dumpfile = fopen(ACC_REC, "w");
	fwrite(users, sizeof(user), MAX_USERS, dumpfile);
	fwrite(groups, sizeof(group), MAX_GROUPS, dumpfile);
	fclose(dumpfile);
	flush_all();
};
//load file system from disk.
int load_acnt() {
	FILE *dumpfile;
	flush_all();
	dumpfile = fopen(ACC_REC, "r");
	if (dumpfile == NULL)
		return -1;
	else {
		fread(users, sizeof(user), MAX_USERS, dumpfile);
		fread(groups, sizeof(group), MAX_GROUPS, dumpfile);
		fclose(dumpfile);
		return 0;
	};
	flush_all();
};
int new_user(char usr[24], char psw[24]) {
	if (usr[0] == '\0')
		return -3;
	int c;
	char n[24];
	char p[24];
	strcpy(n, usr);
	strcpy(p, psw);
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].name) == 0)
			return c;
	};
	for (c = 0; c<MAX_USERS; c++) {
		if (strlen(users[c].name) == 0) {
			strcpy(users[c].name, n);
			strcpy(users[c].psw, p);
			strcpy(users[c].intro, "info not set.\n");
			return -1;
		};
	};
	return -2;
};
int user_to_line(char user[24]) {
	int c;
	char tomatch[24];
	strcpy(tomatch, user);
	for (c = 0; c<MAX_ONLINE; c++) {
		if (strcmp(tomatch, online[c].user) == 0)
			return c;
	}
	return -1;
};
int get_user_idx(char user[24]) {
	int c;
	char n[24];
	strcpy(n, user);
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].name) == 0)
			return c;
	};
	return -1;
};
int user_auth(char usr[24], char psw[24]) {
	char n[24], p[24];
	strcpy(n, usr);
	strcpy(p, psw);
	for (int c = 0; c<MAX_USERS; c++) {
		if ((strcmp(n, users[c].name) == 0) && (strcmp(p, users[c].psw) == 0)) {
			if (user_to_line(n) == -1)
				return 0;   //pass
			else
				return 1;   //already online
		}
	};
	return -2;      //wrong name or passwd
};
int host_group(char ngrp[24], char psw[24]) {
	int c;
	char n[24], p[24];
	strcpy(n, ngrp);
	strcpy(p, psw);
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strlen(groups[c].ngrp) == 0) {
			strcpy(groups[c].ngrp, n);
			strcpy(groups[c].psw, p);
			return c;
		};
	};
	return -1;
};
int get_group_idx(char group[24]) {
	char n[24];
	int c;
	strcpy(n, group);
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strcmp(groups[c].ngrp, n) == 0)
			return c;
	};
	return -1;
};
int join_group(int idx, char usr[24], char psw[24]) {
	char n[24], p[24];
	int c;
	strcpy(n, usr);
	strcpy(p, psw);
	for (c = 0; c<MAX_MENBERS_PER_GROUP; c++) {
		if (strcmp(groups[idx].members[c], n) == 0)
			return -1;
	};
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strlen(groups[idx].members[c]) == 0)) {
			if (strcmp(groups[idx].psw, p) == 0) {
				strcpy(groups[idx].members[c], n);
				return c;
			}
			else
				return -2;
		};
	};
	return -3;
};
int leave_group(int idx, char usr[24]) {
	char n[24];
	int c;
	strcpy(n, usr);
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strcmp(groups[idx].members[c], n) == 0)) {
			strcpy(groups[idx].members[c], "");
			if (c == 0)
				strcpy(groups[idx].ngrp, "");
			return 0;
		};
	};
	return -1;
};
