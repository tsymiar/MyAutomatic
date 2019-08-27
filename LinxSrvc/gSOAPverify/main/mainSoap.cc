//
#include"../util/String_-inl.h"
#include"mainSoap.h"
#include"../sql/sqlofDB.h"
#include"../sys/sysstatus.h"
#include"../soap/soapStub.h"
#include"../soap/myweb.nsmap"

pthread_mutex_t queue_lock;      // 队列锁
pthread_cond_t  queue_noti;      // 条件变量
SOAP_SOCKET     queue[MAX_QUEUE];// 数组队列
int head = 0, tail = 0;          // 队列头队列尾初始化         
void *process_queue(void *);     // 线程入口函数
int enqueue(SOAP_SOCKET, unsigned long ip); // 入队列函数
SOAP_SOCKET dequeue(void);       // 出队列函数
unsigned long dequeue_ip();
static unsigned long ips[MAX_QUEUE];
int logcnt = 0;

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

int enqueue(SOAP_SOCKET sock, unsigned long ip)
{
    int status = SOAP_OK;
    int next;
    pthread_mutex_lock(&queue_lock);
    next = (tail + 1) % MAX_QUEUE;
    if (next >= MAX_QUEUE)
        next = 0;
    // 队列满
    if (next == head)
        status = SOAP_EOM;
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

int http_get(struct soap *soap)
#ifdef NS_HTTPPOST
{
    soap_response(soap, SOAP_HTML);
    soap_send(soap, "<html>Hello I'm WebService.</html>");
    soap_end_send(soap);
    return SOAP_OK;
}
int http_post(struct soap *soap, const char *endpoint, const char *host, int port, const char *path, const char *action, size_t count)
#endif
{
    FILE* stream = 0;
#ifdef NS_HTTPPOST
    // 请求WSDL时，传送相应文件
    // 获取请求的wsdl文件
    std::string fielPath(soap->path);
    size_t pos = fielPath.rfind("/");
    std::string fileName(fielPath, pos + 1);
    // 将?替换为.
    size_t dotPos = fileName.rfind("?");
    if ((int)dotPos == -1)
        return 404;
    fileName.replace(dotPos, 1, ".");
    // 打开WSDL文件准备拷贝
    stream = fopen(fileName.c_str(), "rb");
#else
    char* s = strchr(soap->path, '?');
    if (!s || strcmp(s, "?wsdl"))
        return SOAP_GET_METHOD;
    stream = fopen("myweb.wsdl", "rb");
#endif // NS_HTTPPOST
    if (!stream)
    {
        // HTTP not found error
        return 404;
    }
    // HTTP header with text/xml content
    soap->http_content = "text/xml";
    soap_response(soap, SOAP_FILE);
    for (;;)
    {
        // 从stream中读取数据
        size_t r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), stream);
        if (!r)break;
        if (soap_send_raw(soap, soap->tmpbuf, r))
        {
            fprintf(stderr, "can't send raw data of tmpbuf.\n");
            break;
        }
    }
    fclose(stream);
    soap_end_send(soap);
    return
#ifdef NS_HTTPPOST
        http_get(soap);
#else
        SOAP_OK;
#endif
}

int main_server(int argc, char** argv)
{
#ifdef NS_DEBUG
    argc = 3; argv[1] = (char*)"8080";
#endif // NS_DEBUG
    if (argv[1] == nullptr)
    {
        std::cout << "请输入端口参数 例如：“\033[45m./gSOAPverify 8080\033[0m”\n" << argv[1] << std::endl;
        kill(getppid(), SIGALRM);
        return -1;
    }
    struct soap Soap;
    // 初始化运行时环境
    soap_init(&Soap);
#ifdef NS_HTTPPOST
    Soap.fpost = http_post;
#else
    Soap.fget = http_get;
#endif // !NS_HTTPPOST
    // 设置UTF-8编码
    soap_set_mode(&Soap, SOAP_C_UTFSTRING);
    soap_set_namespaces(&Soap, namespaces);
    // 如果没有参数，当作CGI程序处理
    if (argc < 2)
    {
        // CGI 风格服务请求，单线程
        soap_serve(&Soap);
        // 清除序列化类实例
        soap_destroy(&Soap);
        // 清除序列化数据
        soap_end(&Soap);
    } else
    {
        struct soap * soap_thr[MAX_THR];
        pthread_t tid[MAX_THR];
        int i, port = atoi(argv[1]);
        SOAP_SOCKET m, cs;
        // 锁和条件变量初始化
        pthread_mutex_init(&queue_lock, NULL);
        pthread_cond_init(&queue_noti, NULL);
        // 绑定服务端口
        m = soap_bind(&Soap, NULL, port, BACKLOG);
        int vilid = 0;
        // 循环绑定直至服务套接字合法
        while (!soap_valid_socket(m))
        {
            if (vilid == 0)
            {
                fprintf(stderr, "Bind PORT(%d) \033[31merror\033[0m! \n", port);
                exit(1);
            }
            m = soap_bind(&Soap, NULL, port, BACKLOG);
            vilid++;
        }
        fprintf(stderr, "======== Socket端口号:%s ========\n", argv[1]);

        // 生成服务线程
        for (i = 0; i < MAX_THR; i++)
        {
            soap_thr[i] = soap_copy(&Soap);
            fprintf(stderr, " ++++\tthread %d.\n", i);
            pthread_create(&tid[i], NULL, (void*(*)(void*))process_queue, (void*)soap_thr[i]);
            usleep(50);
        }
        int j = 0;
        for (;;)
        {
            // 接受客户端连接
            cs = soap_accept(&Soap);
            if (!soap_valid_socket(cs))
            {
                if (Soap.errnum)
                {
                    soap_print_fault(&Soap, stderr);
                    continue;
                } else
                {
                    fprintf(stderr, "Server timed out \n");
                    break;
                }
            }
            logcnt++;
            // 客户端IP地址
            fprintf(stderr, "\033[32mAccepted\033[0m \033[1mREMOTE\033[0m connection. IP = \033[33m%d.%d.%d.%d\033[0m, socket = %d, log(%d) \n", \
                (int)(((Soap.ip) >> 24) & 0xFF), (int)(((Soap.ip) >> 16) & 0xFF), (int)(((Soap.ip) >> 8) & 0xFF), \
                (int)((Soap.ip) & 0xFF), (int)(Soap.socket), logcnt);
            // 套接字入队，如果队列已满则循环等待
            while (enqueue(cs, ips[j]) == SOAP_EOM)
                usleep(1000);
            j++;
            if (j >= MAX_THR)
                j = 0;
        }
        // 清理服务
        for (i = 0; i < MAX_THR; i++)
        {
            while (enqueue(SOAP_INVALID_SOCKET, ips[i]) == SOAP_EOM)
            {
                usleep(1000);
            }
        }
        for (i = 0; i < MAX_THR; i++)
        {
            fprintf(stderr, "Waiting for thread %d to terminate ..\n", i);
            pthread_join((pthread_t)tid[i], NULL);
            fprintf(stderr, "terminated \n");
            soap_done(soap_thr[i]);
            free(soap_thr[i]);
        }
        pthread_mutex_destroy(&queue_lock);
        pthread_cond_destroy(&queue_noti);
    }
    // 分离运行时环境
    soap_done(&Soap);
    return 0;
}

int api__trans(struct soap *soap, char* msg, char* rtn[])
{
    int j = 0;
    String_ str;
    struct PARAM {
        char key[16];
        char value[16];
    };
    char*    curstr[4] = { NULL };
    char*    token = NULL;
    struct PARAM params[8];
    char *text[8] = { msg };
    int noeq = str.charcount_(*text, '=');
    printf("GET:[%s][%d]\n", msg, noeq);
    token = strtok(msg, "@&");
    for (int i = 0; i < 8; i++) {
        text[i] = (char*)malloc(64);
    }
    if (memcmp(token, "trans", 6) != 0 || strlen(token) > strlen("trans"))
    {
        memset(text[0], 0, 64);
        memcpy(text[0], "illegal command!", 17);
        return -1;
    }
    while (token != NULL)
    {
        curstr[j] = token;
        if (strstr(curstr[j], "=") != NULL)
        {
            str.strcut_((unsigned char*)curstr[j], '=', params[j].key, params[j].value);
            j++;
        }
        token = strtok(NULL, "&");
    }
    for (int k = 0; k < j; k++)
    {
        sprintf(text[k], "param(%d):%s[%s]", k, params[k].key, params[k].value);
    }
    j = 0;
    token = NULL;
    rtn = (char**)malloc(64);
    memcpy(rtn, *text, 64);
    cout << rtn << endl;
    return 0;
}

int api__get_server_status(struct soap *soap, xsd_string cmd, xsd_string& status)
{
    st_sys ss = { 0 };
    char gt[8];
    if (memcmp(cmd, "1000", 5) == 0)
        show_memory((char*)"localhost", &ss);
    status = gcvt(100.f*ss.mem_free / ss.mem_all, 5, gt);
    cout << status << endl;
    return 0;
}

int api__login_by_key(struct soap*, char *usr, char *psw, struct ArrayOfEmp2 &ccc)
{
    struct queryInfo info;
    ccc.rslt.flag = -3;
    if (!(usr == nullptr || psw == nullptr))
    {
        if (sqlQuery(0, usr, psw, &info) != 0) {
            info.flg = false;
            ccc.rslt.flag = -2;
            printf("[OUT]:\tqueryInfo.rslt is null.\n");
        }
        if (info.flg)
        {
            ccc.rslt.email = info.msg->email;
            ccc.rslt.tell = info.msg->tell;
            printf("[OUT]:\temail:%s\t", ccc.rslt.email);
            if (strlen(ccc.rslt.tell) != 0)
                cout << "tell:" << ccc.rslt.tell;
            cout << endl;
            ccc.rslt.flag = 200;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (fork() == 0)
        main_server(argc, argv);
    return 0;
}
