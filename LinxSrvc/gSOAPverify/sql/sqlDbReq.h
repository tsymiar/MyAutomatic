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
// 启用查询路径 SQL 注入防护
#define FIX
// 最大 SQL 语句长度
#define MAX_SQL_LEN 512

typedef struct st_usr_msg {
    int   age;
    char  sex[3];
    char  tell[14];
    char  email[32];
    bool  flag;
    char* text;
    void* P;
} USR_MSG;

typedef struct st_usr_auth {
    char* acc;
    char* psw;
} USR_AUTH;

struct queryParam {
    USR_AUTH user;
    USR_MSG  msg;
};

#ifdef __cplusplus
extern "C" {
#endif

    int  sqlQuery(struct queryParam& param, bool flag = false);
    void sqlClose();

#ifdef __cplusplus
}
#endif

#endif
