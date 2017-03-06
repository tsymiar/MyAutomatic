#ifndef _SQLDB_H
#define _SQLDB_H
#include <sys/socket.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>
typedef struct st_usr_msg
{
	int age;
	char sex[3];
	char tell[14];
	char email[32];
	char* text;
	void* P;
} USR_MSG;

struct DBinfo {
	bool flg;
	int idx;
	USR_MSG* msg;
};
//#pragma comment (lib,"ws2_32.lib")
//#pragma comment(lib,"mysql/libmysql.lib")

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
	int sqlDB(int type, char* acc, char* psw, struct DBinfo* info);
	void sql_close();
#  ifdef __cplusplus
}
#  endif
#endif
