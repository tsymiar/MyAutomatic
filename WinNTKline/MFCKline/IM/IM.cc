#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#ifdef _WIN32
#pragma comment(lib, "WS2_32.lib")
#include <winsock2.h>
#include <windows.h>
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
#endif
#define DEFAULT_PORT 8877
#define MAX_USERS 200
#define MAX_ONLINE 100
#define MAX_MENBERS_PER_GROUP 20
#define MAX_GROUPS 20
#define ACC_REC "acnts"
//using namespace std;
//C++11 std与socket.h中bind函数冲突
int  aimtoquit;
#ifdef _WIN32
WSADATA wsaData;
SOCKET
#else
int
#endif
listen_socket, recv_socket, send_socket;
#ifdef __linux
pthread_mutexattr_t attr;
#endif
#ifdef _WIN32
CRITICAL_SECTION
#else
pthread_mutex_t
#endif
sendallow;

struct user {
	char name[24];
	char passwd[24];
	char intro[224];
}users[MAX_USERS];

struct group {
	char name[24];
	char passwd[24];
	char member[MAX_MENBERS_PER_GROUP][24];
}groups[MAX_GROUPS];

struct LINE {
	char name[24];
#ifdef _WIN32
	SOCKET
#else
	int
#endif
		sock_r;
#ifdef _WIN32
	SOCKET
#else
	int
#endif
		sock_s;
} online[MAX_ONLINE];

void save_acnt();
int load_acnt(void);
int new_user(char name[24], char passwd[24]);
int user_to_line(char user[24]);
int get_user_idx(char name[24]);
int user_auth(char name[24], char passwd[24]);
int host_group(char group[24], char passwd[24]);
int get_group_idx(char group[24]);
int join_group(int idx, char name[24], char passwd[24]);
int leave_group(int idx, char user[24]);

#ifdef __linux
void *
#else
unsigned __stdcall
#endif
monite(void *arg)
{
#ifdef _WIN32
	SOCKET
#else
	int
#endif
		r_sock = recv_socket,//接收
		s_sock = send_socket;//发送
	int c, rtn, loggedon = 0;
	int qq = 1;
	char tmp[256], bufs[256], name[24];
	do {
		rtn = recv(r_sock, tmp, 256, 0);
		if (rtn == -1) {
			goto con_err1;
		};
#ifdef DEBUGINFO
		printf("1rcv[%0x,%0x]:%s\n", tmp[0], tmp[1], tmp + 8);
#endif  // tmp inc--8 bit crc_head, 24 bit username, 24 bit password.
		if ((tmp[0] == 0) && (tmp[1] == 0)) {
			bufs[0] = bufs[1] = 0;
			rtn = new_user((tmp + 8), (tmp + 32));
			if (rtn == (bufs[2] = -1)) {
				printf("new user: %s\n", (tmp + 8));
				join_group(0, (tmp + 8), (char*)"all");
			}
			else if (rtn == (bufs[2] = -2)) {
				strcpy((bufs + 8), "too many users");
			}
			else {
				bufs[2] = -3;
				strcpy((bufs + 8), "user already exists");
			};
			send(s_sock, bufs, strlen(bufs) + 1, 0);
#ifdef DEBUGINFO
			printf("bufs[%0x,%0x]:%s\n", bufs[0], bufs[1], bufs + 8);
#endif
		}
		else if ((tmp[0] == 0) && (tmp[1] == 0x1)) {
			bufs[0] = 0;
			bufs[1] = 1;
			bufs[2] = rtn = user_auth((tmp + 8), (tmp + 32));
			strcpy(name, (tmp + 8));
			if (rtn == 0) {
				printf("%s logged on\n", (tmp + 8));
				loggedon = 1;
			}
			else if (rtn == (bufs[1] = 1)) {
				strcpy((tmp + 8), "already online");
			}
			else {
				strcpy((tmp + 8), "wrong name or password");
			};
			send(s_sock, bufs, 256, 0);
		}
	} while (loggedon == 0);
	for (c = 0; c<MAX_ONLINE; c++) {
		if (strlen(online[c].name) == 0) {
			online[c].sock_r = r_sock;
			online[c].sock_s = s_sock;
			strcpy(online[c].name, tmp + 8);
			break;
		};
	};
	do {
		rtn = recv(r_sock, tmp, 256, 0);
		if (!(rtn == 256)) {
			printf("lost connection with %s.\n", name);
			goto con_err;
		};
#ifdef DEBUGINFO
		printf("2rcv[%x,%x]:%s\n", tmp[0], tmp[1], tmp + 8);
#endif
		memcpy(bufs, tmp, 2);
		if (tmp[0] == 0)
		{
			switch (tmp[1])
			{
			case 0x3:
			{
				bufs[2] = 0;
				loggedon = 0;
				send(s_sock, bufs, 256, 0);
				printf("%s quit \n", name);
			} break;
			case 0x4:
			{
				bufs[2] = 0;
				rtn = get_user_idx(name);
				strcpy(users[rtn].intro, (tmp + 32));
				send(s_sock, bufs, 256, 0);
			} break;
			case 0x5:
			{
				bufs[2] = 0;
				rtn = get_user_idx(name);
				strcpy(users[rtn].passwd, (tmp + 32));
				send(s_sock, bufs, 256, 0);
			} break;
			case 0x6:
			{
				bufs[2] = 0;
				strcpy((bufs + 8), "onlien user list:");
				send(s_sock, bufs, 256, 0);
				for (c = 0; c < MAX_ONLINE; c++) {
					if (strlen(online[c].name) >> 0) {
						bufs[2] = c;
						strcpy((bufs + 8), online[c].name);
						send(s_sock, bufs, 256, 0);
					};
				};
			} break;
			case 0x7:
			{
				bufs[2] = 0;
				strcpy((bufs + 8), "groups list:");
				send(s_sock, bufs, 256, 0);
				for (c = 0; c < MAX_GROUPS; c++) {
					if (strlen(groups[c].name) >> 0) {
						bufs[1] = c;
						strcpy((bufs + 8), groups[c].name);
						send(s_sock, bufs, 256, 0);
					};
				};
			} break;
			case 0x8:
			{
				bufs[2] = rtn = get_group_idx(tmp + 8);
				if (rtn == -1) {
					strcpy((bufs + 8), "no such group");
				}
				else {
					strcpy((bufs + 8), "users of this group:");
					for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
						if (strlen(groups[rtn].member[c]) >> 0) {
							strcpy((bufs + 8 * (c + 1)), groups[rtn].member[c]);
						};
					};
				};
				send(s_sock, bufs, 256, 0);
			} break;
			case 0x9:
			{
				bufs[2] = rtn = host_group((tmp + 8), (tmp + 32));
				if (rtn == -1) {
					strcpy((bufs + 8), "rejected");
				}
				else {
					printf("new group: %s\n", (tmp + 8));
					join_group(rtn, name, (tmp + 32));
				}
				send(s_sock, bufs, 256, 0);
			} break;
			case 0xA:
			{
				bufs[2] = rtn = join_group(get_group_idx(tmp + 8), name, (tmp + 32));
				if (rtn == -1) {
					strcpy((bufs + 8), "you have already in this group");
				}
				else if (rtn == -2) {
					strcpy((bufs + 8), "wrong password to this group");
				}
				send(s_sock, bufs, 256, 0);
			} break;
			case 0xB:
			{
				bufs[2] = rtn = leave_group(get_group_idx(tmp + 8), name);
				if (rtn == 0)
					strcpy((bufs + 8), "leave group successfully");
				else
					strcpy((bufs + 8), "you are not in this group");
				send(s_sock, bufs, 256, 0);
			} break;
			case 0xC:
			{
				bufs[2] = rtn = get_user_idx(tmp + 8);
				if (rtn >= 0) {
					strcpy((bufs + 8), (tmp + 8));
					strcpy((bufs + 32), users[rtn].intro);
				}
				else {
					strcpy((bufs + 8), "system info: no such user! ");
				}
				send(s_sock, bufs, 256, 0);
			} break;
			case 0xD:
			{ //loop1
				bufs[2] = rtn = get_group_idx(tmp + 8);
				if (rtn == -1) {//rtn=-1
					if (-1 != (bufs[2] = user_to_line(tmp + 8))) {
						strcpy((bufs + 8), name);
						strcpy((bufs + 32), (tmp + 32));
						send(online[rtn].sock_s, bufs, 256, 0);
					};    //loop0
				}//rtn=-1
				else {
					for (c = 0; c < MAX_MENBERS_PER_GROUP; c++) {
						if (!strlen(groups[rtn].member[c]) == 0)
						{
							bufs[2] = -3;
							int sss = user_to_line(groups[rtn].member[c]);
							if (!(sss == -1)) {
								strcpy((bufs + 8), name);
								strcpy((bufs + 32), (tmp + 32));
								send(online[sss].sock_s, bufs, 256, 0);
							}//if sss
						}//if strlen
					}//for
					send(s_sock, bufs, 256, 0);
				}//else
			} break;
			}
		}
	} while (loggedon == 1);
con_err:
	rtn = user_to_line(name);
	strcpy(online[rtn].name, "");

con_err1:
#ifdef _WIN32
	closesocket(r_sock);
	closesocket(s_sock);
#else
	close(r_sock);
	close(s_sock);
#endif
	return NULL;
};

#ifdef __linux
void *
#else
unsigned __stdcall
#endif
commands(void *arg)
{
	char optionstr[24], name[24];
	int rtn;
	do {
		scanf("%s", (char*)&optionstr);
		if (strcmp(optionstr, "quit") == 0) {
#ifdef _WIN32
			closesocket(listen_socket);
#else
			close(listen_socket);
#endif
			printf("saving accounts data to file %s\n", ACC_REC);
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
#ifdef _WIN32
				closesocket(online[rtn].sock_r);
				closesocket(online[rtn].sock_s);
#else
				close(online[rtn].sock_r);
				close(online[rtn].sock_s);
#endif
				printf("user %s kicked !\n", name);
			};
		};
	} while (!(aimtoquit));
	return NULL;
};

int _im_(int argc, char *argv[]) {

	if (!(load_acnt()))
		printf("accounts data loaded from %s\n", ACC_REC);
	else {
		strcpy(users[0].name, "admin");
		strcpy(users[0].passwd, "admin");
		strcpy(groups[0].name, "all");
		strcpy(groups[0].passwd, "all");
		strcpy(groups[0].member[0], "admin");
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
	unsigned int
#else
	pthread_t
#endif
		thread_ID;
#ifdef __linux
	pthread_create(&thread_ID, NULL, commands, (void*)-1);
#else
#ifdef _WIN32
	_beginthreadex(NULL, 0, commands, NULL, 0, &thread_ID);
#endif
#endif
	int err =
#ifdef _WIN32
		WSAStartup(0x202, &wsaData);
#else
		0;
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
	printf("listening to port %d\n", DEFAULT_PORT);
	struct sockaddr_in from;
#ifdef _WIN32
	int
#else
	socklen_t
#endif
		fromlen = (
#ifdef _WIN32
			int
#else
			socklen_t
#endif
			)sizeof(from);
	int c = 0;
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
			printf("accept() OK.\n");
#ifdef DEBUGINFO
		printf("1: %d\n", recv_socket);
#endif
		send_socket = accept(listen_socket, (struct sockaddr*)&from, &fromlen);

		if (send_socket
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
#ifdef __linux
		pthread_create(&thread_ID, NULL, monite, (void*)-1);
#else
		_beginthreadex(NULL, 0, monite, NULL, 0, &thread_ID);
#endif
		c++;
#ifdef DEBUGINFO
		{
			printf("2: %d\n", send_socket);
		}
#endif
		} while (!(aimtoquit));
		printf("c = %d\n", c);
		return 0;
		}

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

//save accounts to file.
void save_acnt() {
#ifdef _WIN32
	_flushall();
#else
	fflush(stdin);
#endif
	FILE *dumpfile;
	dumpfile = fopen(ACC_REC, "w");
	fwrite(users, sizeof(user), MAX_USERS, dumpfile);
	fwrite(groups, sizeof(group), MAX_GROUPS, dumpfile);
	fclose(dumpfile);
#ifdef _WIN32
	_flushall();
#else
	fflush(stdin);
#endif
};
//load file system from disk.
int load_acnt(void) {
	FILE *dumpfile;
#ifdef _WIN32
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
#ifdef _WIN32
	_flushall();
#else
	fflush(stdin);
#endif
};
int new_user(char name[24], char passwd[24]) {
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
int user_to_line(char user[24]) {
	int c;
	char tomatch[24];
	strcpy(tomatch, user);
	for (c = 0; c<MAX_ONLINE; c++) {
		if (strcmp(tomatch, online[c].name) == 0)
			return c;
	}
	return -1;
};
int get_user_idx(char name[24]) {
	int c;
	char n[24];
	strcpy(n, name);
	for (c = 0; c<MAX_USERS; c++) {
		if (strcmp(n, users[c].name) == 0)
			return c;
	};
	return -1;
};
int user_auth(char name[24], char passwd[24]) {
	int c;
	char n[24], p[24];
	strcpy(n, name);
	strcpy(p, passwd);
	for (c = 0; c<MAX_USERS; c++) {
		if ((strcmp(n, users[c].name) == 0) && (strcmp(p, users[c].passwd) == 0)) {
			if (user_to_line(n) == -1)
				return 0;   //pass
			else
				return 1;   //already online
		}
	};
	return 2;      //wrong name or passwd
};
int host_group(char group[24], char passwd[24]) {
	char n[24], p[24];
	int c;
	strcpy(n, group);
	strcpy(p, passwd);
	for (c = 0; c<MAX_GROUPS; c++) {
		if (strlen(groups[c].name) == 0) {
			strcpy(groups[c].name, n);
			strcpy(groups[c].passwd, p);
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
		if (strcmp(groups[c].name, n) == 0)
			return c;
	};
	return -1;
};
int join_group(int idx, char name[24], char passwd[24]) {
	char n[24], p[24];
	int c;
	strcpy(n, name);
	strcpy(p, passwd);
	for (c = 0; c<MAX_MENBERS_PER_GROUP; c++) {
		if (strcmp(groups[idx].member[c], n) == 0)
			return -1;
	};
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strlen(groups[idx].member[c]) == 0)) {
			if (strcmp(groups[idx].passwd, p) == 0) {
				strcpy(groups[idx].member[c], n);
				return c;
			}
			else
				return -2;
		};
	};
	return -3;
};
int leave_group(int idx, char user[24]) {
	char n[24];
	int c;
	strcpy(n, user);
	for (c = 0; c<MAX_GROUPS; c++) {
		if ((strcmp(groups[idx].member[c], n) == 0)) {
			strcpy(groups[idx].member[c], "");
			if (c == 0)
				strcpy(groups[idx].name, "");
			return 0;
		};
	};
	return -1;
};
