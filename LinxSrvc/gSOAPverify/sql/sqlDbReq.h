#ifndef _SQLOFDB_H
#define _SQLOFDB_H

#include <sys/socket.h>
#ifdef HAVE_MYSQL
#include <mysql/mysql.h>
#endif
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

#ifdef HAVE_MYSQL
#ifdef __cplusplus
extern "C" {
#endif

    // sqlQuery 内部完成一次性初始化（并发安全）；命中返回 0，其余见 .cc 注释
    int  sqlQuery(struct queryParam& param);
    // 停看门狗 + 关连接 + 释放库资源，退出前调用一次
    void sqlClose();

#ifdef __cplusplus
}
#endif
#endif  // HAVE_MYSQL

#endif
