#ifndef _SQLOFDB_H
#define _SQLOFDB_H
#include <sys/socket.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#define get_var(v)  (#v)
//#pragma comment(lib,"ws2_32.lib")
//#pragma comment(lib,"mysql/libmysql.lib")
#define FIX

typedef struct st_usr_msg {
	int age;
	char sex[3];
	char tell[14];
	char email[32];
	char* text;
	void* P;
} USR_MSG;

typedef struct st_raw {
	int type;
	char* acc;
	char* psw;
} RAW;

struct queryInfo {
	bool flg;
	int idx;
	RAW raw;
	USR_MSG* msg;
};

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
	int sqlQuery(int type, char* acc, char* psw, struct queryInfo* info);
	void sqlClose();
#  ifdef __cplusplus
}
#  endif
#endif
