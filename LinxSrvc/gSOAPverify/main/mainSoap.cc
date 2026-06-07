//
#include "../../include/String_-inl.h"
#include "mainSoap.h"
#include "../sql/sqlDbReq.h"
#include "../sys/status.h"
#ifdef NS_DBG
#define SOAP_DEBUG
#endif
#include "../soap/soapStub.h"
#include "../soap/myweb.nsmap"

pthread_mutex_t  queue_lock;          // 队列互斥锁
pthread_cond_t   queue_cond;          // 条件变量
soap_socket_t    queue[MAX_QUEUE];    // 环形缓冲区
int              head = 0;            // 队列头
int              tail = 0;            // 队列尾
unsigned long    ips[MAX_QUEUE];      // 客户端 IP 记录

void* process_queue(void* soap)
{
    if (soap == nullptr)
        return nullptr;
    struct soap* serv = (struct soap*)soap;
    for (;;) {
        serv->socket = dequeue();
        dequeue(serv->ip);
        if (!soap_valid_socket(serv->socket)) {
            fprintf(stderr, "Thread %d terminating\n", (int)(long)serv->user);
            break;
        }
        soap_serve(serv);
        soap_destroy(serv);
        soap_end(serv);
    }
    return NULL;
}

int enqueue(soap_socket_t sock, unsigned long ip)
{
    int status = SOAP_OK;
    pthread_mutex_lock(&queue_lock);
    int next = tail + 1;
    if (next >= MAX_QUEUE)
        next = 0;
    if (next == head) {
        status = SOAP_EOM;  // 队列已满
    } else {
        queue[tail] = sock;
        ips[tail] = ip;
        tail = next;
        pthread_cond_signal(&queue_cond);
    }
    pthread_mutex_unlock(&queue_lock);
    return status;
}

soap_socket_t dequeue()
{
    pthread_mutex_lock(&queue_lock);
    while (head == tail) {
        pthread_cond_wait(&queue_cond, &queue_lock);
    }
    soap_socket_t sock = queue[head];
    head++;
    if (head >= MAX_QUEUE)
        head = 0;
    pthread_mutex_unlock(&queue_lock);
    return sock;
}

void dequeue(unsigned int& ip)
{
    // 读取刚出队元素对应的 IP（head 已在 dequeue() 中递增）
    int idx = (head == 0) ? (MAX_QUEUE - 1) : (head - 1);
    ip = ips[idx];
}

// ---------- WSDL 文件安全读取 ----------
static FILE* open_wsdl_safe(const char* path)
{
    // 安全检查：拒绝含 ".."、"//"、绝对路径的请求
    if (path == nullptr || strstr(path, "..") != nullptr
        || strstr(path, "//") != nullptr || path[0] == '/') {
        fprintf(stderr, "[SEC] path traversal blocked: %s\n", path ? path : "(null)");
        return nullptr;
    }
    // 只允许 .wsdl 文件
    const char* dot = strrchr(path, '.');
    if (dot == nullptr || strcasecmp(dot, ".wsdl") != 0) {
        fprintf(stderr, "[SEC] non-wsdl file blocked: %s\n", path);
        return nullptr;
    }
    return fopen(path, "rb");
}

int http_get(struct soap* soap)
#ifdef NS_HTTPPOST
{
    soap_response(soap, SOAP_HTML);
    soap_send(soap, "<html>Hello I'm WebService.</html>");
    soap_end_send(soap);
    return SOAP_OK;
}
int http_post(struct soap* soap, const char* endpoint, const char* host,
    int port, const char* path, const char* action, size_t count)
#endif
{
    FILE* stream = nullptr;
#ifdef NS_HTTPPOST
    // 从请求路径提取文件名并安全检查
    std::string filePath(soap->path);
    size_t pos = filePath.rfind("/");
    std::string fileName(filePath, pos + 1);
    // 将 ? 替换为 .
    size_t dotPos = fileName.rfind("?");
    if (dotPos == std::string::npos)
        return SOAP_404;
    fileName.replace(dotPos, 1, ".");
    stream = open_wsdl_safe(fileName.c_str());
#else
    char* s = strchr(soap->path, '?');
    if (!s || strcmp(s, "?wsdl"))
        return SOAP_GET_METHOD;
    stream = open_wsdl_safe("myweb.wsdl");
#endif
    if (!stream) {
        return SOAP_404;
    }
    // 发送 WSDL XML
    soap->http_content = "text/xml";
    soap_response(soap, SOAP_FILE);
    for (;;) {
        size_t r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), stream);
        if (!r) break;
        if (soap_send_raw(soap, soap->tmpbuf, r)) {
            fprintf(stderr, "can't send raw data of tmpbuf.\n");
            break;
        }
    }
    fclose(stream);
    soap_end_send(soap);
#ifdef NS_HTTPPOST
    return http_get(soap);
#else
    return SOAP_OK;
#endif
}

int main_server(int argc, char** argv)
{
    if (argc < 2 || argv[1] == nullptr) {
        std::cout << "Please type an argument as port eg. '\033[45m"
            << argv[0] << " 8800\033[0m'" << std::endl;
        kill(getppid(), SIGALRM);
        return -1;
    }

    struct soap Soap;
    soap_init(&Soap);
#ifdef NS_HTTPPOST
    Soap.fpost = http_post;
#else
    Soap.fget = http_get;
#endif
    soap_set_mode(&Soap, SOAP_C_UTFSTRING);
    soap_set_namespaces(&Soap, namespaces);

    struct timespec ts = { 0, 50000 };

    // 无参数: CGI 模式
    if (argc < 2) {
        soap_serve(&Soap);
        soap_destroy(&Soap);
        soap_end(&Soap);
    } else {
        // 独立服务器模式
        struct soap* soap_thr[MAX_THR];
        pthread_t    tid[MAX_THR];

        pthread_mutex_init(&queue_lock, NULL);
        pthread_cond_init(&queue_cond, NULL);

        // 绑定端口
        int port = atoi(argv[1]);
        soap_socket_t m = soap_bind(&Soap, NULL, port, BACKLOG);
        int valid = 0;
        while (!soap_valid_socket(m)) {
            if (valid == 0) {
                fprintf(stderr, "Bind PORT(%d) \033[31merror\033[0m!\n", port);
                exit(1);
            }
            m = soap_bind(&Soap, NULL, port, BACKLOG);
            valid++;
        }
        fprintf(stdout, "======== Socket Server Port: %d ========\n", port);

        // 创建工作线程池
        for (int i = 0; i < MAX_THR; i++) {
            soap_thr[i] = soap_copy(&Soap);
            fprintf(stderr, " ++++\tthread %d.\n", i);
            pthread_create(&tid[i], NULL, process_queue, (void*)soap_thr[i]);
            nanosleep(&ts, NULL);
        }

        // 主循环: 接受连接并分发
        int j = 0;
        static int no = 0;
        for (;;) {
            soap_socket_t sock = soap_accept(&Soap);
            if (!soap_valid_socket(sock)) {
                if (Soap.errnum) {
                    soap_print_fault(&Soap, stderr);
                    continue;
                } else {
                    fprintf(stderr, "Server timed out\n");
                    break;
                }
            }
            no++;
            fprintf(stdout,
                "\033[32mAccepted\033[0m \033[1mREMOTE\033[0m connection. "
                "IP = \033[33m%d.%d.%d.%d\033[0m, socket = %d, log(%d)\n",
                (int)(((Soap.ip) >> 24) & 0xFF),
                (int)(((Soap.ip) >> 16) & 0xFF),
                (int)(((Soap.ip) >> 8) & 0xFF),
                (int)((Soap.ip) & 0xFF),
                (int)(Soap.socket), no);

            // 入队，满则等待
            while (enqueue(sock, ips[j]) == SOAP_EOM) {
                ts.tv_nsec = 100000;
                nanosleep(&ts, NULL);
            }
            j++;
            if (j >= MAX_THR)
                j = 0;
        }

        // 发送停止信号
        for (int i = 0; i < MAX_THR; i++) {
            while (enqueue(SOAP_INVALID_SOCKET, ips[i]) == SOAP_EOM) {
                ts.tv_nsec = 100000;
                nanosleep(&ts, NULL);
            }
        }
        // 等待线程终止
        for (int i = 0; i < MAX_THR; i++) {
            fprintf(stderr, "Waiting for thread %d to terminate ..\n", i);
            pthread_join(tid[i], NULL);
            fprintf(stderr, "terminated\n");
            soap_done(soap_thr[i]);
            free(soap_thr[i]);
        }
        pthread_mutex_destroy(&queue_lock);
        pthread_cond_destroy(&queue_cond);
        soap_done(&Soap);
    }
    return 0;
}

// ==================== API: trans — 通用数据转发 ====================
// 修复: 移除 text[0] 内存覆写、memset(text[0]) 循环 bug、内存泄漏
int api__trans(struct soap* soap, char* msg, char* rtn[])
{
    static const int MAX_PARAM = 8;
    static const int KEY_LEN = 16;
    static const int VAL_LEN = 16;
    static const int BUF_LEN = 128;

    String_s ss;
    struct PARAM {
        char key[KEY_LEN];
        char value[VAL_LEN];
    };

    if (msg == nullptr || msg[0] == '\0') {
        static char err_buf[] = "request uri empty!";
        *rtn = err_buf;
        return -1;
    }

    // 拷贝 msg 避免 strtok 修改原始数据
    char msg_copy[BUF_LEN] = { 0 };
    strncpy(msg_copy, msg, BUF_LEN - 1);

    int neq = ss.char_count_(msg_copy, '=');
    if (neq < 0) {
        static char err_buf[] = "request uri error!";
        *rtn = err_buf;
        return -1;
    }
    printf("GET:[%s][%d]\n", msg_copy, neq);

    // 分配输出缓冲区 (修复: 原 memset(text[0]) 循环 bug)
    static char text_buf[MAX_PARAM][BUF_LEN];
    memset(text_buf, 0, sizeof(text_buf));

    // 解析命令名
    char* token = strtok(msg_copy, "@&");
    if (token == nullptr || memcmp(token, "trans", 6) != 0) {
        snprintf(text_buf[0], BUF_LEN, "illegal command!");
        *rtn = text_buf[0];
        return -2;
    }

    // 解析 key=value 参数
    struct PARAM params[MAX_PARAM];
    memset(params, 0, sizeof(params));
    int p_cnt = 0;
    token = strtok(NULL, "&");
    while (token != nullptr && p_cnt < MAX_PARAM) {
        if (strchr(token, '=') != nullptr) {
            ss.strcut_((unsigned char*)token, '=',
                params[p_cnt].key, params[p_cnt].value);
            snprintf(text_buf[p_cnt], BUF_LEN,
                "Param(%d): %s[%s]", p_cnt,
                params[p_cnt].key, params[p_cnt].value);
            cout << text_buf[p_cnt] << endl;
            p_cnt++;
        }
        token = strtok(NULL, "&");
    }

    *rtn = text_buf[0];
    return 0;
}

// ==================== API: get-server-status — 获取服务器状态 ====================
int api__get_server_status(struct soap* soap, xsd_string req, xsd_string& rsp)
{
    if (req != nullptr && memcmp(req, "1000", 5) == 0) {
        st_sys ss = {};
        char gt[16];
        get_mem_stat("localhost", &ss);
        // 用 snprintf 替代已弃用的 gcvt
        snprintf(gt, sizeof(gt), "%.5g",
            ss.mem_all > 0 ? (100.0 * ss.mem_free / ss.mem_all) : 0.0);
        rsp = gt;
        cout << req << ": " << rsp << endl;
    }
    return 0;
}

// ==================== API: login-by-key — 用户登录认证 ====================
int api__login_by_key(struct soap*, char* usr, char* psw,
    struct api__ArrayOfEmp2& sch)
{
    sch.rslt.flag = -3;
    if (usr != nullptr && psw != nullptr) {
        struct queryParam param;
        memset(&param, 0, sizeof(param));
        param.user.acc = usr;
        param.user.psw = psw;
        // sqlQuery 内部通过指针修改 param.msg (修复: 原按值传递无效)
        int ret = sqlQuery(param);
        if (ret != 0) {
            param.msg.flag = false;
            sch.rslt.flag = -2;
            printf("[OUT]:\tqueryParam.rslt is null.\n");
        }
        if (param.msg.flag) {
            sch.rslt.email = param.msg.email;
            sch.rslt.tell = param.msg.tell;
            sch.rslt.flag = 200;
            printf("[OUT]:\temail:%s\t", sch.rslt.email);
            if (sch.rslt.tell[0] != '\0')
                cout << "tell:" << sch.rslt.tell;
            cout << endl;
        }
    }
    return sch.rslt.flag;
}

int main(int argc, char* argv[])
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }
    if (pid == 0) {
        // 子进程: 运行 SOAP 服务
        // 创建新会话，脱离终端
        setsid();
        return main_server(argc, argv);
    }
    // 父进程: 等待子进程避免僵尸进程
    printf("gSOAPverify daemon started, PID = %d\n", pid);
    int status;
    waitpid(pid, &status, 0);
    return 0;
}
