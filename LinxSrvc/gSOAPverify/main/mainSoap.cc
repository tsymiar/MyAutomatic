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

    // 超时（秒）。不设默认就是 0 = 无限：慢客户端 / 半开连接会长期占住 worker，
    // 8 个线程很快被耗尽。worker 的 soap 由 soap_copy(&Soap) 得到，会继承这三个值。
    Soap.accept_timeout = TIMEOUT_SEC;   // 等待新连接
    Soap.recv_timeout   = TIMEOUT_SEC;   // 接收请求
    Soap.send_timeout   = TIMEOUT_SEC;   // 发送响应

    struct timespec ts = { 0, 50000 };

    // 入口已校验 argc >= 2，此处固定以独立服务器模式运行
    {
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
        static int no = 0;
        for (;;) {
            soap_socket_t sock = soap_accept(&Soap);
            if (!soap_valid_socket(sock)) {
                if (Soap.errnum) {
                    soap_print_fault(&Soap, stderr);
                }
                // accept 超时（errnum 为 0，属正常空闲）与可恢复错误都继续等待。
                // 这里不能 break：设了 accept_timeout 后，空闲 TIMEOUT_SEC 秒就会
                // 触发一次超时，break 会让服务自己退出。
                continue;
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

            // 入队，满则等待。IP 必须传本次连接的真实地址：
            // 原先传 ips[j] 只是槽位里的旧值/未初始化值，记录到的客户端 IP 全是错的
            while (enqueue(sock, Soap.ip) == SOAP_EOM) {
                ts.tv_nsec = 100000;
                nanosleep(&ts, NULL);
            }
        }

        // 发送停止信号
        for (int i = 0; i < MAX_THR; i++) {
            while (enqueue(SOAP_INVALID_SOCKET, 0) == SOAP_EOM) {
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
// 注意: gSOAP 服务函数返回非 0 会被当成错误、不发响应体（客户端只收到 Fault），
//       所以出错时也要返回 SOAP_OK，把错误说明放在出参 *rtn 里。
//       解析必须用 strtok_r（strtok 用内部静态状态，8 个 worker 并发会互相打断），
//       输出缓冲区必须是栈上临时缓冲 + soap_strdup（原 static text_buf 会被并发覆盖）。
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

    if (rtn == nullptr)
        return SOAP_OK;
    if (msg == nullptr || msg[0] == '\0') {
        *rtn = soap_strdup(soap, "request uri empty!");
        return SOAP_OK;
    }

    // 拷贝 msg 避免解析时修改原始数据
    char msg_copy[BUF_LEN];
    size_t msg_len = strnlen(msg, BUF_LEN);
    if (msg_len == BUF_LEN) {
        *rtn = soap_strdup(soap, "request uri too long!");
        return SOAP_OK;
    }
    memcpy(msg_copy, msg, msg_len);
    msg_copy[msg_len] = '\0';

    int neq = ss.char_count_(msg_copy, '=');
    if (neq < 0) {
        *rtn = soap_strdup(soap, "request uri error!");
        return SOAP_OK;
    }
    printf("GET:[%s][%d]\n", msg_copy, neq);

    char out[BUF_LEN] = { 0 };
    char line[BUF_LEN];

    // 解析命令名
    char* saveptr = nullptr;
    char* token = strtok_r(msg_copy, "@&", &saveptr);
    if (token == nullptr || strcmp(token, "trans") != 0) {
        *rtn = soap_strdup(soap, "illegal command!");
        return SOAP_OK;
    }

    // 解析 key=value 参数
    struct PARAM params[MAX_PARAM];
    memset(params, 0, sizeof(params));
    int p_cnt = 0;
    while ((token = strtok_r(nullptr, "&", &saveptr)) != nullptr
        && p_cnt < MAX_PARAM) {
        if (strchr(token, '=') != nullptr) {
            ss.strcut_((unsigned char*)token, '=',
                params[p_cnt].key, params[p_cnt].value);
            snprintf(line, BUF_LEN, "Param(%d): %s[%s]", p_cnt,
                params[p_cnt].key, params[p_cnt].value);
            cout << line << endl;
            if (p_cnt == 0)
                memcpy(out, line, strlen(line) + 1);
            p_cnt++;
        }
    }

    *rtn = soap_strdup(soap, out);
    return SOAP_OK;
}

// ==================== API: get-server-status — 获取服务器状态 ====================
int api__get_server_status(struct soap* soap, xsd_string req, xsd_string& rsp)
{
    // 用 strcmp 而非 memcmp(req,"1000",5)：后者固定读 5 字节，req 短于 4 字符会越界读
    if (req != nullptr && strcmp(req, "1000") == 0) {
        st_sys ss = {};
        char gt[16];
        get_mem_stat("localhost", &ss);
        // 用 snprintf 替代已弃用的 gcvt
        snprintf(gt, sizeof(gt), "%.5g",
            ss.mem_all > 0 ? (100.0 * ss.mem_free / ss.mem_all) : 0.0);
        // 用 soap 上下文分配，避免把栈上数组地址返回给调用方
        rsp = soap_strdup(soap, gt);
        cout << req << ": " << rsp << endl;
    }
    return 0;
}

// ==================== API: login-by-key — 用户登录认证 ====================
// 注意: gSOAP 要求服务函数返回 SOAP_OK(0)，返回任何非 0 值都会被当成错误、
//       直接中断且不发送响应体（客户端只会收到 Fault）。
//       认证结果必须放在出参 sch.rslt.flag 里返回。
int api__login_by_key(struct soap* soap, char* usr, char* psw,
    struct api__ArrayOfEmp2& sch)
{
    sch.rslt.flag = -3;
    sch.rslt.email = nullptr;
    sch.rslt.tell  = nullptr;
    if (usr != nullptr && psw != nullptr && usr[0] != '\0' && psw[0] != '\0') {
        struct queryParam param;
        memset(&param, 0, sizeof(param));
        param.user.acc = usr;
        param.user.psw = psw;
        // sqlQuery 内部通过指针修改 param.msg (修复: 原按值传递无效)
        int ret = sqlQuery(param);
        if (ret != 0) {
            param.msg.flag = false;
            sch.rslt.flag = -2;
            printf("[OUT]:\tqueryParam.rslt is null (ret=%d).\n", ret);
        }
        if (param.msg.flag) {
            // param 是栈上变量，直接保存它的数组成员地址会变成悬垂指针；
            // 必须用 soap 上下文分配，生命周期才覆盖到本次响应序列化结束。
            sch.rslt.email = soap_strdup(soap, param.msg.email);
            sch.rslt.tell  = soap_strdup(soap, param.msg.tell);
            sch.rslt.flag = 200;
            printf("[OUT]:\temail:%s\t", sch.rslt.email ? sch.rslt.email : "(null)");
            if (sch.rslt.tell && sch.rslt.tell[0] != '\0')
                cout << "tell:" << sch.rslt.tell;
            cout << endl;
        }
    }
    return SOAP_OK;
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
