//
#include "../../include/String_-inl.h"
#include "mainSoap.h"
#include "../sql/sqlDbReq.h"
#include "../sys/status.h"
#ifdef NS_DBG
#define SOAP_DEBUG
#endif
#include <sys/socket.h>
#include <sys/utsname.h>
#include <cerrno>
#include "../soap/soapStub.h"
#include "../soap/myweb.nsmap"

pthread_mutex_t  queue_lock;          // 队列互斥锁
pthread_cond_t   queue_cond;          // 条件变量
soap_socket_t    sock_queue[MAX_QUEUE]; // 环形缓冲区
int              head = 0;            // 队列头
int              tail = 0;            // 队列尾
unsigned long    ips_list[MAX_QUEUE]; // 客户端 IP 记录

// 优雅退出：SIGINT/SIGTERM 置位，主循环与看门狗据此收尾
static volatile sig_atomic_t g_stop = 0;
static soap_socket_t         g_master = SOAP_INVALID_SOCKET;  // 监听 socket
static pid_t                 g_child = -1;                   // 父进程持有子进程 PID

extern "C" void on_signal_stop(int sig)
{
    g_stop = 1;
    // 关掉监听 socket，让阻塞在 soap_accept 的主线程立刻返回，不必等满超时
    if (soap_valid_socket(g_master)) {
        shutdown(g_master, SHUT_RDWR);
    }
    // 父进程（等待者）把信号转给真正干活的子进程
    if (g_child > 0) {
        kill(g_child, sig);
    }
}

static void install_signal_handlers()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal_stop;
    sigemptyset(&sa.sa_mask);
    // 不设 SA_RESTART：让 accept/recv 立即被 EINTR 打断，尽快退出
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void* process_queue(void* soap)
{
    if (soap == nullptr)
        return nullptr;
    struct soap* serv = (struct soap*)soap;
    for (;;) {
        serv->socket = dequeue(&serv->ip);
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
        sock_queue[tail] = sock;
        ips_list[tail] = ip;
        tail = next;
        pthread_cond_signal(&queue_cond);
    }
    pthread_mutex_unlock(&queue_lock);
    return status;
}

soap_socket_t dequeue(unsigned int* ip)
{
    pthread_mutex_lock(&queue_lock);
    while (head == tail) {
        pthread_cond_wait(&queue_cond, &queue_lock);
    }
    soap_socket_t sock = sock_queue[head];
    // IP 必须和 socket 在同一次加锁里取，否则会读到别的连接对应的 IP
    if (ip != nullptr) {
        *ip = ips_list[head];
    }
    head++;
    if (head >= MAX_QUEUE)
        head = 0;
    pthread_mutex_unlock(&queue_lock);
    return sock;
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

// 命令名前缀（GET 查询串用），与 SOAP 操作名 api__get_server_status 对应
#define HTTP_CMD_STATUS "get-server-status"

// ---------- GET 形式的状态查询 ----------
// 让 ?wsdl 之外的 GET 也能取到服务器状态，地址栏写法与 trans 的
// "<cmd>@k=v" 保持一致，同时接受标准查询串写法：
//   http://host:8800/?get-server-status@req=100
//   http://host:8800/?get-server-status&req=100
static int http_get_server_status(struct soap* soap, const char* query)
{
    char buf[128] = { 0 };
    snprintf(buf, sizeof(buf), "%s", query);

    // 按 "@" 与 "&" 切段，取 req 的值（找不到就是空串）。
    // ram / cpu 也可以直接写成查询键：?get-server-status@ram=1
    char req[32] = { 0 };
    char* saveptr = nullptr;
    for (char* tk = strtok_r(buf, "@&", &saveptr); tk != nullptr;
        tk = strtok_r(nullptr, "@&", &saveptr)) {
        if (strncmp(tk, "req=", 4) == 0) {
            snprintf(req, sizeof(req), "%s", tk + 4);
            break;
        }
        if (strncmp(tk, "ram", 3) == 0) {
            snprintf(req, sizeof(req), "ram");
            break;
        }
        if (strncmp(tk, "cpu", 3) == 0) {
            snprintf(req, sizeof(req), "cpu");
            break;
        }
    }

    // 复用 SOAP 接口的实现：req 不是 "100" 时 rsp 保持为空
    char* rsp = nullptr;
    api__get_server_status(soap, req, rsp);

    soap->http_content = "text/plain; charset=utf-8";
    if (rsp == nullptr || rsp[0] == '\0') {
        static const char hint[] =
            "usage: ?" HTTP_CMD_STATUS "@req=100|ram|cpu\n";
        // SOAP_FILE + 400 = HTTP 400（soap_response 会把 status 换算成状态码）
        soap_response(soap, SOAP_FILE + 400);
        soap_send_raw(soap, hint, sizeof(hint) - 1);
        soap_end_send(soap);
        return SOAP_OK;
    }
    char line[64];
    int len = snprintf(line, sizeof(line), "%s\n", rsp);
    soap_response(soap, SOAP_FILE);
    if (len > 0) {
        soap_send_raw(soap, line, (size_t)len);
    }
    soap_end_send(soap);
    return SOAP_OK;
}

int http_get(struct soap* soap)
#ifdef NS_HTTPPOST
{
    char* q = strchr(soap->path, '?');
    if (q != nullptr && strncmp(q + 1, HTTP_CMD_STATUS,
        sizeof(HTTP_CMD_STATUS) - 1) == 0) {
        return http_get_server_status(soap, q + 1);
    }
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
    if (s == nullptr) {
        return SOAP_GET_METHOD;
    }
    // GET 形式的状态查询（?wsdl 之外的唯一可用 GET）
    if (strncmp(s + 1, HTTP_CMD_STATUS, sizeof(HTTP_CMD_STATUS) - 1) == 0) {
        return http_get_server_status(soap, s + 1);
    }
    if (strcmp(s + 1, "wsdl") != 0) {
        return SOAP_GET_METHOD;
    }
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

static void setup_soap(struct soap* soap)
{
#ifdef NS_HTTPPOST
    soap->fpost = http_post;
#else
    soap->fget = http_get;
#endif
    soap_set_mode(soap, SOAP_C_UTFSTRING);
    soap_set_namespaces(soap, namespaces);

    // 超时（秒）。不设默认就是 0 = 无限：慢客户端 / 半开连接会长期占住 worker，
    // 8 个线程很快被耗尽。worker 的 soap 由 soap_copy() 得到，会继承这三个值。
    soap->accept_timeout = TIMEOUT_SEC;   // 等待新连接
    soap->recv_timeout = TIMEOUT_SEC;   // 接收请求
    soap->send_timeout = TIMEOUT_SEC;   // 发送响应
}

static void bind_port(struct soap* soap, int port)
{
    soap_socket_t m = soap_bind(soap, NULL, port, BACKLOG);
    int valid = 0;
    while (!soap_valid_socket(m)) {
        if (valid == 0) {
            fprintf(stderr, "Bind PORT(%d) \033[31merror\033[0m!\n", port);
            exit(1);
        }
        m = soap_bind(soap, NULL, port, BACKLOG);
        valid++;
    }
    fprintf(stdout, "======== Socket Server Port: %d ========\n", port);
}

static void start_workers(struct soap* soap, struct soap* thr[], pthread_t tid[])
{
    struct timespec ts = { 0, 50000 };
    for (int i = 0; i < MAX_THR; i++) {
        thr[i] = soap_copy(soap);
        // 记下线程号，process_queue 退出时打印用
        thr[i]->user = (void*)(long)i;
        fprintf(stderr, " ++++\tthread %d.\n", i);
        pthread_create(&tid[i], NULL, process_queue, (void*)thr[i]);
        nanosleep(&ts, NULL);
    }
}

static void accept_loop(struct soap* soap)
{
    static int no = 0;
    struct timespec ts = { 0, 100000 };
    for (;;) {
        if (g_stop) {
            break;
        }
        soap_socket_t sock = soap_accept(soap);
        if (!soap_valid_socket(sock)) {
            if (g_stop) {
                break;
            }
            if (soap->errnum) {
                soap_print_fault(soap, stderr);
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
            (int)(((soap->ip) >> 24) & 0xFF),
            (int)(((soap->ip) >> 16) & 0xFF),
            (int)(((soap->ip) >> 8) & 0xFF),
            (int)((soap->ip) & 0xFF),
            (int)(soap->socket), no);

        // 入队，满则等待。IP 必须传本次连接的真实地址
        while (enqueue(sock, soap->ip) == SOAP_EOM) {
            nanosleep(&ts, NULL);
        }
    }
}

static void stop_workers(struct soap* soap, struct soap* thr[], pthread_t tid[])
{
    struct timespec ts = { 0, 100000 };
    // 发送停止信号（每个 worker 一个非法 socket，各自取到就退出）
    fprintf(stderr, "Shutting down, waiting for workers ..\n");
    for (int i = 0; i < MAX_THR; i++) {
        while (enqueue(SOAP_INVALID_SOCKET, 0) == SOAP_EOM) {
            nanosleep(&ts, NULL);
        }
    }
    for (int i = 0; i < MAX_THR; i++) {
        pthread_join(tid[i], NULL);
        soap_done(thr[i]);
        free(thr[i]);
    }
}

int main_server(int argc, char** argv)
{
    if (argc < 2 || argv[1] == nullptr) {
        std::cout << "Please type an argument as port eg. '\033[45m"
            << argv[0] << " 8800\033[0m'" << std::endl;
        // 退出码交给 main() 带回给父进程
        return -1;
    }

    install_signal_handlers();

    struct soap Soap;
    soap_init(&Soap);
    setup_soap(&Soap);

    // 入口已校验 argc >= 2，此处固定以独立服务器模式运行
    struct soap* soap_thr[MAX_THR];
    pthread_t    tid[MAX_THR];

    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&queue_cond, NULL);
    bind_port(&Soap, atoi(argv[1]));
    start_workers(&Soap, soap_thr, tid);
    g_master = Soap.master;
    accept_loop(&Soap);
    stop_workers(&Soap, soap_thr, tid);
    pthread_mutex_destroy(&queue_lock);
    pthread_cond_destroy(&queue_cond);
    soap_done(&Soap);
#ifdef HAVE_MYSQL
    // 收尾：停掉 MySQL 看门狗并关闭连接
    sqlClose();
#endif
    return 0;
}

// ==================== API: trans — 通用数据转发 ====================
// 注意: gSOAP 服务函数返回非 0 会被当成错误、不发响应体（客户端只收到 Fault），
//       所以出错时也要返回 SOAP_OK，把错误说明放在出参 *rtn 里。
//       解析必须用 strtok_r（strtok 用内部静态状态，8 个 worker 并发会互相打断），
//       输出缓冲区必须是栈上临时缓冲 + soap_strdup（原 static text_buf 会被并发覆盖）。
static const int TRANS_MAX_PARAM = 8;
static const int TRANS_KEY_LEN = 16;
static const int TRANS_VAL_LEN = 16;
static const int TRANS_BUF_LEN = 128;

struct TransParam {
    char key[TRANS_KEY_LEN];
    char value[TRANS_VAL_LEN];
};

/* 解析 "trans@key=value&..."；返回参数个数，命令名非法返回 -1 */
static int parseTransParams(char* msg, struct TransParam* params, char* out, size_t outLen)
{
    char line[TRANS_BUF_LEN];
    char* saveptr = nullptr;
    char* token = strtok_r(msg, "@&", &saveptr);
    if (token == nullptr || strcmp(token, "trans") != 0) {
        return -1;
    }
    String_s ss;
    int cnt = 0;
    while ((token = strtok_r(nullptr, "&", &saveptr)) != nullptr
        && cnt < TRANS_MAX_PARAM) {
        if (strchr(token, '=') != nullptr) {
            ss.strcut_((unsigned char*)token, '=',
                params[cnt].key, params[cnt].value);
            snprintf(line, TRANS_BUF_LEN, "Param(%d): %s[%s]", cnt,
                params[cnt].key, params[cnt].value);
            cout << line << endl;
            if (cnt == 0)
                // 用 snprintf 代替 memcpy+strlen: 自带上界, 不会因 line 缺少
                // '\0' 结尾而越界读 (CWE-126)
                snprintf(out, outLen, "%s", line);
            cnt++;
        }
    }
    return cnt;
}

int api__trans(struct soap* soap, char* msg, char* rtn[])
{
    if (rtn == nullptr)
        return SOAP_OK;
    if (msg == nullptr || msg[0] == '\0') {
        *rtn = soap_strdup(soap, "request uri empty!");
        return SOAP_OK;
    }

    // 拷贝 msg 避免解析时修改原始数据
    char msg_copy[TRANS_BUF_LEN];
    size_t msg_len = strnlen(msg, TRANS_BUF_LEN);
    if (msg_len == TRANS_BUF_LEN) {
        *rtn = soap_strdup(soap, "request uri too long!");
        return SOAP_OK;
    }
    memcpy(msg_copy, msg, msg_len);
    msg_copy[msg_len] = '\0';

    String_s ss;
    int neq = ss.char_count_(msg_copy, '=');
    if (neq < 0) {
        *rtn = soap_strdup(soap, "request uri error!");
        return SOAP_OK;
    }
    printf("GET:[%s][%d]\n", msg_copy, neq);

    char out[TRANS_BUF_LEN] = { 0 };
    struct TransParam params[TRANS_MAX_PARAM];
    memset(params, 0, sizeof(params));
    if (parseTransParams(msg_copy, params, out, sizeof(out)) < 0) {
        *rtn = soap_strdup(soap, "illegal command!");
        return SOAP_OK;
    }

    *rtn = soap_strdup(soap, out);
    return SOAP_OK;
}

// ==================== API: get-server-status — 获取服务器状态 ====================
// req 取值决定返回什么：
//   "100"  空闲内存百分比
//   "ram"  物理内存总大小
//   "cpu"  CPU 核数与架构
int api__get_server_status(struct soap* soap, xsd_string req, xsd_string& rsp)
{
    if (req == nullptr) {
        return 0;
    }
    st_sys ss = { };
    char gt[128] = { 0 };   // 放得下 "cpu=N arch=<machine>"（machine 最长 65）
    // 用 strcmp 而非 memcmp(req,"100",5)：后者固定读 5 字节，
    // req 短于 4 字符会越界读
    if (strcmp(req, "100") == 0) {
        get_mem_stat("localhost", &ss);
        // 用 snprintf 替代已弃用的 gcvt
        snprintf(gt, sizeof(gt), "%.5g",
            ss.mem_all > 0 ? (100.0 * ss.mem_free / ss.mem_all) : 0.0);
    } else if (strcmp(req, "ram") == 0) {
        get_mem_stat("localhost", &ss);
        snprintf(gt, sizeof(gt), "ram=%lldMB", ss.mem_all);
    } else if (strcmp(req, "cpu") == 0) {
        get_mem_stat("localhost", &ss);
        struct utsname uts;
        if (uname(&uts) != 0) {
            snprintf(gt, sizeof(gt), "cpu=%ld", ss.li_cpu);
        } else {
            snprintf(gt, sizeof(gt), "cpu=%ld arch=%s", ss.li_cpu, uts.machine);
        }
    } else {
        return 0;
    }
    // 用 soap 上下文分配，避免把栈上数组地址返回给调用方
    rsp = soap_strdup(soap, gt);
    cout << req << ": " << rsp << endl;
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
    sch.rslt.tell = nullptr;
    if (usr != nullptr && psw != nullptr && usr[0] != '\0' && psw[0] != '\0') {
        struct queryParam param;
        memset(&param, 0, sizeof(param));
        param.user.acc = usr;
        param.user.psw = psw;
#ifdef HAVE_MYSQL
        // sqlQuery 通过引用修改 param.msg
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
            sch.rslt.tell = soap_strdup(soap, param.msg.tell);
            sch.rslt.flag = 200;
            printf("[OUT]:\temail:%s\t", sch.rslt.email ? sch.rslt.email : "(null)");
            if (sch.rslt.tell && sch.rslt.tell[0] != '\0')
                cout << "tell:" << sch.rslt.tell;
            cout << endl;
        }
#else
        // 未启用 MySQL，无法查库认证
        sch.rslt.flag = -4;
#endif
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
        // 创建新会话，脱离终端。这里故意不 chdir("/")：
        // ?wsdl 要按相对路径读 myweb.wsdl，切根目录会直接 404。
        setsid();
        return main_server(argc, argv);
    }
    // 父进程: 等待子进程避免僵尸进程，并把收到的终止信号转给子进程
    g_child = pid;
    install_signal_handlers();
    printf("gSOAPverify daemon started, PID = %d\n", pid);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        // 被信号打断继续等，直到子进程真的退出
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
