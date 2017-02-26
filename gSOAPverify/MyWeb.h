#ifndef MYWEB_H
#define MYWEB_H
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/errno.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
//#include <corecrt_io.h>

#define OFFSET 10240

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
