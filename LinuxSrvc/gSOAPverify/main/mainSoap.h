#ifndef mainSoap_H
#define mainSoap_H

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

#define HAVE_STRUCT_TIMESPEC

#define  BACKLOG (64)  
#define  MAX_THR (8)   
#define  MAX_QUEUE (1024)
#define  OFFSET 10240

#endif
