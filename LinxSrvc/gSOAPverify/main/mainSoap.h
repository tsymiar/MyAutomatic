#ifndef mainSoap_H
#define mainSoap_H

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>

#define HAVE_STRUCT_TIMESPEC

#define BACKLOG    (64)    // listen() 积压连接数
#define MAX_THR    (8)     // 工作线程数
#define MAX_QUEUE  (1024)  // 环形缓冲区容量
#define SOAP_404   404     // HTTP 404 状态码

// SOAP_SOCKET 由 stdsoap2.h 定义(通常为 int)，本头文件不依赖 SOAP 头文件
typedef int soap_socket_t;

extern pthread_mutex_t  queue_lock;
extern pthread_cond_t   queue_cond;
extern soap_socket_t    queue[MAX_QUEUE];
extern int              head, tail;
extern unsigned long    ips[MAX_QUEUE];

void* process_queue(void*);
int           enqueue(soap_socket_t, unsigned long ip);
soap_socket_t dequeue();
void          dequeue(unsigned int&);
int           main_server(int argc, char** argv);

#endif
