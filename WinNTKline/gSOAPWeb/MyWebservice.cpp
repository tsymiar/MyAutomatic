//
#include<iostream>
#include<winsock2.h>
#include<windows.h> 
#include"sql\sqldb.h"
#define HAVE_STRUCT_TIMESPEC
//#define MY_WSDL
#include"pthreads.2\pthread.h"
#include"soap\web.nsmap"
#include"thdpool\pthread_pool.h"
#pragma comment(lib, "pthreads.2/lib/x64pthreadVC2.lib") 
#pragma comment(lib, "WS2_32.lib")

//宏与全局变量的定义
#define  BACKLOG (64)  
#define  MAX_THR (8)   
#define  MAX_QUEUE (1024)

pthread_mutex_t queue_lock;//队列锁
pthread_cond_t  queue_noti;//条件变量
SOAP_SOCKET     queue[MAX_QUEUE];//数组队列
int head = 0, tail = 0;          //队列头队列尾初始化         
void *process_queue(void *);     //线程入口函数
int enqueue(SOAP_SOCKET, unsigned long ip); //入队列函数
unsigned long dequeue_ip();
SOAP_SOCKET dequeue(void); //出队列函数
static unsigned long ips[MAX_QUEUE];
int http_get(struct soap *soap);
int http_post(struct soap *soap, const char *endpoint, const char *host, int port, const char *path, const char *action, size_t count);

int main(int argc, char** argv)
{
#ifdef MY_DEBUG
	argc = 3; argv[1] = "88";
#endif // MY_DEBUG
	struct soap Soap;
	//初始化运行时环境
	soap_init(&Soap);
	Soap.fget = http_get;
#ifndef MY_WSDL
	Soap.fpost = http_post;
#endif // !MY_WSDL
	//设置UTF-8编码
	soap_set_mode(&Soap, SOAP_C_UTFSTRING);
	soap_set_namespaces(&Soap, namespaces);
	//如果没有参数，当作CGI程序处理
	if (argc <2)
	{
		//CGI 风格服务请求，单线程
		soap_serve(&Soap);
		//清除序列化的类的实例
		soap_destroy(&Soap);
		//清除序列化的数据
		soap_end(&Soap);
	}
	else
	{
		struct soap * soap_thr[MAX_THR];
		pthread_t tid[MAX_THR];
		int i, port = atoi(argv[1]);
		SOAP_SOCKET m, cs;
		//锁和条件变量初始化
		pthread_mutex_init(&queue_lock, NULL);
		pthread_cond_init(&queue_noti, NULL);
		//绑定服务端口
		m = soap_bind(&Soap, NULL, port, BACKLOG);
		//循环直至服务套接字合法
		int vilid = 0;
		while (!soap_valid_socket(m))
		{
			if (vilid == 0)
				fprintf(stderr, "Bind port error! \n");
			m = soap_bind(&Soap, NULL, port, BACKLOG);
			vilid++;
		}
		fprintf(stderr, "Socket端口号:%s \n", argv[1]);

		//生成服务线程
		for (i = 0; i <MAX_THR; i++)
		{
			soap_thr[i] = soap_copy(&Soap);
			fprintf(stderr, "\tthread %d.\n", i);
			pthread_create(&tid[i], NULL, (void*(*)(void*))process_queue, (void*)soap_thr[i]);
		}
		int j = 0;
		for (;;)
		{
			//接受客户端的连接
			cs = soap_accept(&Soap);
			if (!soap_valid_socket(cs))
			{
				if (Soap.errnum)
				{
					soap_print_fault(&Soap, stderr);
					continue;
				}
				else
				{
					fprintf(stderr, "Server timed out \n");
					break;
				}
			}
			//客户端的IP地址
			fprintf(stderr, "Accepted connection from IP= %d.%d.%d.%d socket = %d \n",
				(int)(((Soap.ip) >> 24) && 0xFF), (int)(((Soap.ip) >> 16) & 0xFF), (int)(((Soap.ip) >> 8) & 0xFF), (int)((Soap.ip) & 0xFF), (int)(Soap.socket));
			//请求的套接字进入队列，如果队列已满则循环等待
			while (enqueue(cs, ips[j]) == SOAP_EOM)
				Sleep(1000);
			j++;
			if (j >= MAX_THR)
				j = 0;
		}
		//服务结束后的清理工作
		for (i = 0; i < MAX_THR; i++)
		{
			while (enqueue(SOAP_INVALID_SOCKET, ips[i]) == SOAP_EOM)
			{
				Sleep(1000);
			}
		}
		for (i = 0; i< MAX_THR; i++)
		{
			fprintf(stderr, "Waiting for thread %d to terminate ..\n", i);
			pthread_join(tid[i], NULL);
			fprintf(stderr, "terminated \n");
			soap_done(soap_thr[i]);
			free(soap_thr[i]);
		}
		pthread_mutex_destroy(&queue_lock);
		pthread_cond_destroy(&queue_noti);
	}
	//分离运行时的环境
	soap_done(&Soap);
	return 0;
}

void * process_queue(void * soap)
{
	struct soap * tsoap = (struct soap *)soap;
	for (;;)
	{
		tsoap->socket = dequeue();
		tsoap->ip = dequeue_ip();
		if (!soap_valid_socket(tsoap->socket))
		{
#ifdef DEBUG  
			fprintf(stderr, "Thread %d terminating\n", (int)(long)tsoap->user);
#endif
			break;
		}
		soap_serve(tsoap);
		soap_destroy(tsoap);
		soap_end(tsoap);
	}
	return NULL;
}

//入队列操作
int enqueue(SOAP_SOCKET sock, unsigned long ip)
{
	int status = SOAP_OK;
	int next;
	pthread_mutex_lock(&queue_lock);
	next = (tail + 1) % MAX_QUEUE;
	if (next >= MAX_QUEUE)
		next = 0;
	if (next == head)
		status = SOAP_EOM;//队列满
	else
	{
		queue[tail] = sock;
		ips[tail] = ip;
		tail = next;
		pthread_cond_signal(&queue_noti);
	}
	pthread_mutex_unlock(&queue_lock);
	return status;
}

//出队列操作
SOAP_SOCKET dequeue()
{
	SOAP_SOCKET sock;
	pthread_mutex_lock(&queue_lock);
	while (head == tail)
	{
		pthread_cond_wait(&queue_noti, &queue_lock);
	}
	sock = queue[head++];
	if (head >= MAX_QUEUE)
	{
		head = 0;
	}
	pthread_mutex_unlock(&queue_lock);
	return sock;
}

unsigned long dequeue_ip()
{
	unsigned long ip;
	int num = 0;
	if (head == 0)
		num = MAX_QUEUE - 1;
	else
		num = head - 1;
	ip = ips[num];
	return ip;
}
#ifdef MY_WSDL
int http_get(struct soap *soap)
#else
int http_post(struct soap *soap, const char *endpoint, const char *host, int port, const char *path, const char *action, size_t count)
#endif
{
	FILE* fd = 0;
#ifndef MY_WSDL
	// 请求WSDL时，传送相应文件
	// 获取请求的wsdl文件名
	std::string fielPath(soap->path);
	size_t pos = fielPath.rfind("/");
	std::string fileName(fielPath, pos + 1);

	// 将?替换为.
	size_t dotPos = fileName.rfind("?");
	if (dotPos == -1)
	{
		return 404;
	}
	fileName.replace(dotPos, 1, ".");
	// 打开WSDL文件准备拷贝
	fopen_s(&fd, fileName.c_str(), "rb");
#else
	char* s = strchr(soap->path, '?');
	if (!s || strcmp(s, "?wsdl"))
		return SOAP_GET_METHOD;
	fopen_s(&fd, "web.wsdl", "rb");
#endif // MY_WSDL
	if (!fd)
		// HTTP not found error
		return 404;
	// HTTP header with text/xml content
	soap->http_content = "text/xml";
	soap_response(soap, SOAP_FILE);
	for (;;)
	{
		// 从fd中读取数据
		size_t r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
		if (!r)
		{
			break;
		}
		if (soap_send_raw(soap, soap->tmpbuf, r))
		{
			// can't send, but little we can do about that
			break;
		}
	}
	fclose(fd);
	soap_end_send(soap);
	return
#ifdef MY_WSDL
		SOAP_OK;
#else
		http_get(soap);
}

int http_get(struct soap *soap)
{
	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>Hello I'm WebService.</html>");
	soap_end_send(soap);
	return SOAP_OK;
#endif
}

int api__login_by_key(struct soap *soap, xsd_string usr, xsd_string psw, struct api__result &flag)
{
	int key = 0;
	flag.email = (char*)soap_malloc(soap, 32);
	if (!(strcmp(usr, "0") || strcmp(psw, "0")))
	{
		flag.email = "OK";
		key = 1;
	}
	printf(flag.email);
	return key;
}

int api__encrypt(struct soap *soap, char* input, char** output)
{
	sqldb();
	return 0;
}