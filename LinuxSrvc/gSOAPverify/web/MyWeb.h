#ifndef MYWEB_H
#define MYWEB_H
#include <sys/ioctl.h>  
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/errno.h>
#include <signal.h>
#include <fcntl.h> 
#include <unistd.h>  
#include <cerrno> 
#include <cstdlib> 
#include <cstdio>
#include <pthread.h>
//#include <corecrt_io.h>

#define HAVE_STRUCT_TIMESPEC
#define MY_HTTPGET

//宏与全局变量的定义
#define  BACKLOG (64)  
#define  MAX_THR (8)   
#define  MAX_QUEUE (1024)
#define  OFFSET 10240

class myWeb
{
public:
	int len, file = 0, move = 0;
	unsigned char buff[1024];
	//	char* _file = "./mysql/libmysql.dll";
	//	char* _moving = "../x64/gSOAPWeb/libmysql.dll";
	inline int movedll();
};
#endif
