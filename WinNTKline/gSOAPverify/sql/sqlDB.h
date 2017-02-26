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
struct DBinfo {
	int flg;
	int idx;
	int age;
	char* tell;
	char* email;
};
//#pragma comment (lib,"ws2_32.lib")
//#pragma comment(lib,"mysql/libmysql.lib")

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
	int sqlDB(int type, char* acc, char* psw, struct DBinfo* info);
#  ifdef __cplusplus
}
#  endif
#endif
