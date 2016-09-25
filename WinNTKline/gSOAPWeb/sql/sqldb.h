#ifndef _SQLDB_H
#define _SQLDB_H
#include <winsock.h>
#include <iostream>
#include <string>
#include <mysql.h>

#pragma comment (lib,"ws2_32.lib")
#pragma comment(lib,"sql/libmysql.lib")
#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
	int sqldb();
#  ifdef __cplusplus
}
#  endif
#endif
