#ifndef _SQLDB_H
#define _SQLDB_H
#include <sys/socket.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string>

//#pragma comment (lib,"ws2_32.lib")
//#pragma comment(lib,"mysql/libmysql.lib")

struct DBinfo {
	int id;
	char name[16];
	char psw[16];
	int age;
	char email[32];
	char tele[14];
};

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
	int sqlDB(int type, char* acc, char* psw, DBinfo* info);
#  ifdef __cplusplus
}
#  endif
#endif
