#ifndef _SQLDB_H
#define _SQLDB_H
#include <sys/socket.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <iostream>
#include <cstdlib>
#include <string>

//#pragma comment (lib,"ws2_32.lib")
//#pragma comment(lib,"mysql/libmysql.lib")

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
	int sqlDB(int type, char* acc, char* psw, char* out[]);
#  ifdef __cplusplus
}
#  endif
#endif
