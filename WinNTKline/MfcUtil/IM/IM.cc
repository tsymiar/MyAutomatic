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
#include <signal.h>
#endif
#define THREAD_NUM 20
#define DEFAULT_PORT 8877
#define MAX_USERS 100
#define MAX_ONLINE 30
#define MAX_MENBERS_PER_GROUP 10
#define MAX_GROUPS 10
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
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS
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

type_socket listen_socket;
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
	char usr[24];
	char psw[24];
	char intro[224];
}users[MAX_USERS];

struct group {
	char ngrp[24];
	char pswd[24];
	char members[MAX_MENBERS_PER_GROUP][24];
}groups[MAX_GROUPS];

struct LINE {
	char user[24];
	type_socket sock_rcv;
} online[MAX_ONLINE];


int _im_(int argc, char *argv[]);

void pipesig_handler(int s) {
	printf("Caught SIGPIPE!\n");
}

int main(
#ifdef __linux
	int argc, char* argv[]
#endif
)
{
#ifdef __linux
	if (fork() == 0)
	{
	signal(SIGPIPE, pipesig_handler);
#else
	int argc = 1;
	char* argv[] = { NULL };
#endif
	printf("IM chat server v0.%d for %dbit OS.\n", 1, sizeof(void*) * 8);
	_im_(argc, argv);
#ifdef __linux
	}
#endif
	return 0;
}

int save_acnt();
int load_acnt();
int new_user(char usr[24], char psw[24]);
int user_is_line(char user[24]);
int set_user_line(char user[24], type_socket sock);
int set_user_quit(char user[24]);
int get_user_idx(char user[24]);
int user_auth(char usr[24], char psw[24]);
int host_group(char ngrp[24], char psw[24]);
int get_group_idx(char group[24]);
int join_group(int idx, char usr[24], char psw[24]);
int leave_group(int idx, char usr[24]);

type_thread_func monite(void *socket)
{
	int c, flg, rtn, cnt = 0;
	unsigned int buflen;
	int qq = 0, logged = 0;
	char name[24], bufs[256], rcv_txt[256];
	struct sockaddr_in sin;
	type_len len = (type_len)sizeof(sin);
	type_socket rcv_sock = 0;
	type_socket *sock = (type_socket*)socket;
	if (sock == NULL || *sock == NULL)
		rcv_sock = accept(listen_socket, (struct sockaddr*)&sin, &len);
	else
		rcv_sock = *sock;
	bool set = true;
	setsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&set, sizeof(bool));
	if (rcv_sock
#ifdef _WIN32
		== INVALID_SOCKET) {
		std::cerr << "accept() failed error " << WSAGetLastError();
		WSACleanup();
#else
		< 0) {
#endif
		std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
		return (type_thread_func)-1;
	};
#ifdef _DEBUG
	printf("socket monite: %d\n", rcv_sock);
#endif
	do {
		memset(bufs, 0, 256);
		memset(rcv_txt, 0, 256);
		memset(name, 0, sizeof(name));
		int val;
#ifdef _WIN32
		int
#else
		socklen_t
#endif
			lol = sizeof(int);
		if (getsockopt(rcv_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, &lol) == 0) {
			if (cnt == 3)
				printf("client socket(%d) closed by itself.\n", rcv_sock);
			cnt++;
		}
		flg = recv(rcv_sock, rcv_txt, 256, 0);
		if (qq != flg && flg != 0)
		{
#ifdef _DEBUG
			printf("1-rcv [%0x,%0x]: %s, %d\n", rcv_txt[0], rcv_txt[1], rcv_txt + 8, flg);
			for (c = 0; c < flg; c++)
			{
				if (c % 32 == 0)
					printf("\n");
				printf("%02x ", (unsigned char)rcv_txt[c]);
			}
			printf("\n");
#endif
			// rcv_txt: inc--8 bit crc_head, 24 bit username, 24 bit password.
			if ((rcv_txt[0] == 0) && (rcv_txt[1] == 0)) {
				bufs[0] = bufs[1] = 0;
				rtn = new_user((rcv_txt + 8), (rcv_txt + 32));
				if (rtn == -1) {
					sprintf(bufs + 2, "%x", -1);
					sprintf(bufs + 8, "new user: %s\n", (rcv_txt + 8));
					join_group(0, (rcv_txt + 8), (char*)"all");
				}
				else if (rtn == -2) {
					sprintf(bufs + 2, "%x", -2);
					strcpy((bufs + 8), "too many users.\n");
				}
				else if (rtn >= 0) {
					sprintf(bufs + 2, "%x", rtn);
					strcpy((bufs + 8), "user already exists.\n");
				}
				else if (rtn == -3) {
					sprintf(bufs + 2, "%x", -3);
					strcpy((bufs + 8), "user name error.\n");
				};
				send(rcv_sock, bufs, 64, 0);
#ifdef _DEBUG
				if (flg > 0) printf("bufs[%0x,%0x]:%s\n", bufs[0], bufs[1], bufs + 8);
#endif
				break;
			}
			else if ((rcv_txt[0] == 0) && (rcv_txt[1] == 0x1)) {
				bufs[0] = 0;
				bufs[1] = 0x1;
				// name: 8; password: 32.
				sprintf(bufs + 2, "%x", (rtn = user_auth((rcv_txt + 8), (rcv_txt + 32))));
				strcpy(name, (rcv_txt + 8));
				if (rtn == 1) {
					logged = 1;
					sprintf(bufs + 8, "[%s] logged on.\n", (rcv_txt + 8));
				}
				else if (rtn == 0) {
					strcpy((bufs + 8), "already logged.\n");
				}
				else if (rtn == -1) {
					strcpy((bufs + 8), "check username/password again.\n");
				};
				send(rcv_sock, bufs, 64, 0);
			}
			// set cur qq as last flg;
			qq = flg;
			set_user_line(name, rcv_sock);
			while (logged) {
				flg = recv(rcv_sock, rcv_txt, 256, 0);
				if (flg < 0 && flg != EWOULDBLOCK && flg != EAGAIN && flg != EINTR) {
					printf("%s[%d]\n", strerror(errno), errno);
					goto con_err1;
					break;
				}
				else if (flg == 0) {
					printf("socket disconnect normally.\n");
				}
				if (flg < 24)
					goto con_err;
				if (flg == -1) {
					printf("lost connection with %s.\n", name);
					goto con_err1;
				};
#ifdef _DEBUG
				if (flg > 0) printf("2-rcv [%x,%x]: %s, %d\n", rcv_txt[0], rcv_txt[1], rcv_txt + 8, flg);
				for (c = 0; c < flg; c++)
				{
					if (c % 32 == 0)
						printf("\n");
					printf("%02x ", (unsigned char)rcv_txt[c]);
				}
				printf("\n");
#endif
				buflen = 256;
				memset(bufs, 0, 256);
				if (rcv_txt[0] == 0)
				{
					bufs[1] = rcv_txt[1];
					switch (rcv_txt[1])
					{
					case 0x01:
						strcpy(bufs + 8, "already online.");
						buflen = 24;
						break;
					case 0x2:
						buflen = 8;
						break;
					case 0x3:
					{
						sprintf(bufs + 8, "%s quit.\n", name);
						set_user_quit(name);
						logged = 0;
						send(rcv_sock, bufs, 40, 0);
						goto con_err1;
					} 
					return 0;
					case 0x4:
					{
						rtn = get_user_idx(name);
						sprintf(bufs + 2, "%x", rtn);
						strcpy(users[rtn].intro, (rcv_txt + 32));
						buflen = 8;
					} break;
					case 0x5:
					{
						rtn = get_user_idx(name);
						sprintf(bufs + 2, "%x", rtn);
						if (user_auth((rcv_txt + 8), (rcv_txt + 32)) >= 0)
							strcpy(users[rtn].psw, (rcv_txt + 56));
						else
							strcpy((bufs + 8), "change psswrd fail: user auth error.");
						buflen = 48;
					} break;
					case 0x6:
					{
						strcpy((bufs + 8), "users logged list:");
						for (c = 0; c < MAX_ONLINE; c++) {
							if (strlen(online[c].user) >> 0) {
								sprintf(bufs + 2, "%x", c);
								strcpy((bufs + 8 * (c + 4)), online[c].user);
							}
							if (online[c].user[0] == '\0')
								break;
						};
						buflen = 8 * (c + 4 + 2);
					} break;
					case 0x7:
					{
						strcpy((bufs + 8), "groups list:");
						for (c = 0; c < MAX_GROUPS; c++) {
							if (strlen(groups[c].ngrp) >> 0) {
								sprintf(bufs + 2, "%x", c);
								strcpy((bufs + 8 * (c + 4)), groups[c].ngrp);
							};
							if (groups[c].ngrp[0] == '\0')
								break;
						};
						buflen = 8 * (c + 4 + 2);
					} break;
					case 0x8:
					{
						rtn = get_group_idx(rcv_txt + 8);
						sprintf(bufs + 2, "%x", rtn);
						if (rtn == -1) {
							strcpy((bufs + 8), "no such group.\n");
							buflen = 24;
						}
						else {
							strcpy((bufs + 8), "users of this group:");
							for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
								if (strlen(groups[rtn].members[c]) >> 0) {
									strcpy((bufs + 8 * (c + 4)), groups[rtn].members[c]);
								};
							};
							if (groups[rtn].members[c][0] == '\0')
								break;
							buflen = 8 * (c + 4 + 2);
						};
					} break;
					case 0x9:
					{
						rtn = host_group((rcv_txt + 8), (rcv_txt + 32));
						sprintf(bufs + 2, "%x", rtn);
						if (rtn == -1) {
							strcpy((bufs + 8), "rejected.");
						}
						else {
							join_group(rtn, name, (rcv_txt + 32));
							sprintf(bufs + 8, "new group: %s.\n", (rcv_txt + 8));
						}
						buflen = 24;
					} break;
					case 0xA:
					{
						rtn = join_group(get_group_idx(rcv_txt + 8), name, (rcv_txt + 32));
						sprintf(bufs + 2, "%x", rtn);
						if (rtn == -1) {
							strcpy((bufs + 8), "you have already in this group.\n");
						}
						else if (rtn == -2) {
							strcpy((bufs + 8), "wrong password to this group.\n");
						}
						buflen = 48;
					} break;
					case 0xB:
					{
						rtn = leave_group(get_group_idx(rcv_txt + 8), name);
						sprintf(bufs + 2, "%x", rtn);
						if (rtn == 0)
							strcpy((bufs + 8), "leave group successfully.\n");
						else
							strcpy((bufs + 8), "you are not in this group.\n");
						buflen = 40;
					} break;
					case 0xC:
					{
						rtn = get_user_idx(rcv_txt + 8);
						sprintf(bufs + 2, "%x", rtn);
						strcpy((bufs + 8), (rcv_txt + 8));
						if (rtn >= 0) {
							strcpy((bufs + 32), users[rtn].intro);
							buflen = 260;
						}
						else {
							strcpy((bufs + 32), ":\nNo such user!\n");
							buflen = 56;
						}
					} break;
					case 0xD:
					{ //loop1
						rtn = get_group_idx(rcv_txt + 8);
						sprintf(bufs + 2, "%x", rtn);
						if (rtn == -1) {//rtn=-1
							if (-1 != user_is_line(rcv_txt + 8)) {
								sprintf(bufs + 3, "%x", -1);
								strcpy((bufs + 8), name);
								strcpy((bufs + 32), (rcv_txt + 32));
								send(online[rtn].sock_rcv, bufs, 256, 0);
							};    //loop0
						}//rtn=-1
						else {
							for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
								if (!strlen(groups[rtn].members[c]) == 0)
								{
									sprintf(bufs + 3, "%x", -3);
									int sss = user_is_line(groups[rtn].members[c]);
									if (!(sss == -1)) {
										strcpy((bufs + 8), name);
										strcpy((bufs + 32), (rcv_txt + 32));
										send(online[sss].sock_rcv, bufs, 256, 0);
									}//if sss
								}//if strlen
							}//for
						}//else
					} break; default: break;
					}
					send(rcv_sock, bufs, buflen, 0);
				}
			}
		};
		qq = flg;
#ifdef _WIN32
		Sleep(99);
#elif __linux
		usleep(99999);
#endif
	} while (!logged);

con_err:
	rtn = user_is_line(name);
	strcpy(online[rtn].user, "");

con_err1:
	closesocket(rcv_sock);

	return NULL;
	};

type_thread_func commands(void *arg)
{
	int rtn;
	char optionstr[24], name[24];
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
			rtn = user_is_line(name);
			if (!(rtn == -1)) {
				closesocket(online[rtn].sock_rcv);
				printf("user %s kicked !\n", name);
			};
		};
#ifdef _WIN32
		Sleep(99);
#elif __linux
		usleep(99999);
#endif
	} while (!(aimtoquit));
	return NULL;
};

int _im_(int argc, char *argv[]) {
	int servport = DEFAULT_PORT;
	if (argc == 2 && atoi(argv[1]) != 0) {
		servport = atoi(argv[1]);
	}
	if (!load_acnt()) {
		printf("accounts load finish from [%s].\n", ACC_REC);
	}
	else {
		strcpy(users[0].usr, "admin");
		strcpy(users[0].psw, "test123$");
		strcpy(groups[0].ngrp, "all");
		strcpy(groups[0].pswd, "all");
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
		std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
		return -1;
	}
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(servport);
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
		std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
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
		std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
		return -1;
	}
	printf("listening PORT [%d].\n", servport);
	int cnt = 0;
	do {
#if (defined THREAD_PER_CONN ) || (defined TEST_SOCK)
		struct sockaddr_in from;
		type_len fromlen = (type_len)sizeof(from);
		char IPdotdec[16]; IPdotdec;
		type_socket test_socket = accept(listen_socket, (struct sockaddr*)&from, &fromlen);
#endif
#ifdef TEST_SOCK
		if (test_socket
#ifdef _WIN32
			== INVALID_SOCKET) {
			std::cerr << "accept() failed error " << WSAGetLastError();
			WSACleanup();
#else
			< 0) {
#endif
			std::cerr << "ERROR(" << errno << "): " << strerror(errno) << std::endl;
			return -1;
		}
		else
		{
#ifdef __linux
			inet_ntop(AF_INET, (void *)&from.sin_addr, IPdotdec, 16);
#else
			memcpy(IPdotdec, inet_ntoa(from.sin_addr), 16);
#endif
			printf("accept [%s] success.\n", IPdotdec);
		}
#ifdef _DEBUG
		printf("socket test(%d): val=%d\n", c, test_socket);
#endif
		closesocket(test_socket);
#endif
		if (THREAD_NUM != 0 && cnt == THREAD_NUM)
			break;
#ifdef _WIN32
#ifdef THREAD_PER_CONN
		_beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, (void*)&test_socket, 0, &thread_ID);
#else
		_beginthreadex(NULL, 0, (_beginthreadex_proc_type)monite, NULL, 0, &thread_ID);
#endif
#else
		pthread_create(&thread_ID, NULL, monite, (void*)-1);
#ifdef THREAD_PER_CONN
		pthread_create(&thread_ID, NULL, monite, (void*)&test_socket);
#endif
#endif
		cnt++;
#ifdef _WIN32
		Sleep(99);
#elif __linux
		usleep(99999);
#endif
		} while (!aimtoquit);
		printf("thread count = %d\n", cnt); 
#ifdef _WIN32
		if (cnt == THREAD_NUM)
			while (1);
#endif
		return 0;
	}
//save accounts to file.
int save_acnt() {
	flush_all();
	FILE *dumpfile = NULL;
	dumpfile = fopen(ACC_REC, "w");
	if (dumpfile == NULL)
		return -1;
	fwrite(users, sizeof(user), MAX_USERS, dumpfile);
	fwrite(groups, sizeof(group), MAX_GROUPS, dumpfile);
	if (fclose(dumpfile) != 0)
		return -2;
	flush_all();
	return 0;
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
	char *n = usr;
	char *p = psw;
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].usr) == 0)
			return c;
	};
	for (c = 0; c<MAX_USERS; c++) {
		if (strlen(users[c].usr) == 0) {
			strcpy(users[c].usr, n);
			strcpy(users[c].psw, p);
			strcpy(users[c].intro, "info not set.\n");
			return -1;
		};
	};
	return -2;
};
int user_is_line(char user[24]) {
	int c;
	char *tomatch = user;
	for (c = 0; c<MAX_ONLINE; c++) {
		if (strcmp(tomatch, online[c].user) == 0)
			return c;
	}
	return -1;
};
int set_user_line(char user[24], type_socket sock) {
	for (int i = 0; i < MAX_ONLINE; i++) {
		if (online[i].user[0] == '\0')
		{
			memcpy(online[i].user, user, 24);
			online[i].sock_rcv = sock;
			break;
		}
	}
	return 0;
};
int set_user_quit(char user[24]) {
	char *n = user;
	for (int i = 0; i < MAX_ONLINE; i++) {
		if (strcmp(n, online[i].user) == 0)
		{
			memset(online[i].user, 0, 24);
			online[i].sock_rcv = 0;
			break;
		}
	}
	return 0;
}
int get_user_idx(char user[24]) {
	int c;
	char *n = user;
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].usr) == 0)
			return c;
	};
	return -1;
};
int user_auth(char usr[24], char psw[24]) {
	char *n = usr, *p = psw;
	for (int c = 0; c<MAX_USERS; c++) {
		if ((strcmp(n, users[c].usr) == 0) && (strcmp(p, users[c].psw) == 0)) {
			if (user_is_line(n) == -1) {
				return 1;   //success
			}
			else
				return 0;   //pass
		}
		else
			return -1; //wrong name or passwd
	};
	return 0;
};
int host_group(char ngrp[24], char psw[24]) {
	int c;
	char *n = ngrp, *p = psw;
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strlen(groups[c].ngrp) == 0) {
			strcpy(groups[c].ngrp, n);
			strcpy(groups[c].pswd, p);
			return c;
		};
	};
	return -1;
};
int get_group_idx(char group[24]) {
	int c;
	char *n = group;
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strcmp(groups[c].ngrp, n) == 0)
			return c;
	};
	return -1;
};
int join_group(int idx, char usr[24], char psw[24]) {
	int c;
	char *n = usr, *p = psw;
	for (c = 0; c<MAX_MENBERS_PER_GROUP; c++) {
		if (strcmp(groups[idx].members[c], n) == 0)
			return -1;
	};
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strlen(groups[idx].members[c]) == 0)) {
			if (strcmp(groups[idx].pswd, p) == 0) {
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
	int c;
	char *n = usr;
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
