#include <atomic>

#include "sqlDbReq.h"

using namespace std;

namespace {

    // MySQL 连接配置常量 (放在 struct 外部避免 AARCH64 static constexpr 链接问题)
    constexpr int  SQL_PORT         = 3306;
    constexpr int  MAX_RECONNECT    = 9;
    // 首次查询失败后等看门狗重连再重试的次数（不含首次）
    constexpr int  MAX_RETRY        = 1;
    const     char SQL_HOST[]       = "localhost";
    const     char SQL_USER[]       = "root";
    const     char SQL_DB[]         = "myautomatic";

    struct DbContext {
        MYSQL         mysql;
        int           call_cnt   = 0;
        int           reconnect  = 0;
        bool          connected  = false;
        bool          ready      = false;   // 已初始化（sqlClose 才能执行）
        atomic<bool>  stop{false};          // 通知看门狗退出
        pthread_t     watch_thread;
        bool          watch_running = false;
        pthread_mutex_t sql_lock;
    } g_ctx;

    pthread_once_t g_once = PTHREAD_ONCE_INIT;

    // 注意: 密码从环境变量 MYAUTO_MYSQL_PSW 读取，避免硬编码密码和泄露风险。
    // 未设置时静默返回 nullptr，由调用方决定怎么提示（看门狗只能提示一次，
    // 否则每 3 秒刷一条）。
    const char* get_sql_password()
    {
        const char* env = getenv("MYAUTO_MYSQL_PSW");
        if (env == nullptr || env[0] == '\0') {
            return nullptr;
        }
        return env;
    }

    // ---------- 连接监控线程 ----------
    // mysql_ping / mysql_real_connect 与查询线程共用同一个 MYSQL 句柄，
    // 必须全部串行化：否则 ping 会在 mysql_query 发送中途插入自己的报文，
    // 连接状态直接错乱。
    void* watch_connect(void*)
    {
        bool warned_no_psw = false;
        while (!g_ctx.stop.load()) {
            pthread_mutex_lock(&g_ctx.sql_lock);
            if (mysql_ping(&g_ctx.mysql) != 0) {
                g_ctx.connected = false;
                const char* sql_psw = get_sql_password();
                if (sql_psw == nullptr) {
                    // 口令缺失时看门狗无能为力，只提示一次，避免每 3 秒刷屏
                    if (!warned_no_psw) {
                        cerr << "Reconnect skipped: MySQL password not configured." << endl;
                        warned_no_psw = true;
                    }
                } else if (mysql_real_connect(&g_ctx.mysql,
                    SQL_HOST, SQL_USER,
                    sql_psw,
                    SQL_DB, SQL_PORT,
                    NULL, 0) == NULL) {
                    if (g_ctx.reconnect < MAX_RECONNECT) {
                        cerr << "Reconnect mysql fail: "
                            << mysql_error(&g_ctx.mysql) << "!" << endl;
                        g_ctx.reconnect++;
                    }
                } else {
                    g_ctx.connected = true;
                    g_ctx.reconnect = 0;
                    cout << "[DB] reconnected." << endl;
                }
            }
            pthread_mutex_unlock(&g_ctx.sql_lock);

            // 分片睡眠，stop 置位后最迟 1 秒退出，不必等满 3 秒
            for (int i = 0; i < 3 && !g_ctx.stop.load(); i++) {
                sleep(1);
            }
        }
        return nullptr;
    }

    // ---------- 一次性初始化（pthread_once 保证并发只执行一次且执行完才返回） ----------
    void db_init_once()
    {
        pthread_mutex_init(&g_ctx.sql_lock, NULL);

        if (mysql_library_init(0, NULL, NULL) != 0) {
            cerr << "lib init fail." << endl;
        }
        if (mysql_init(&g_ctx.mysql) == NULL) {
            cerr << "MySQL init fail." << endl;
            return;
        }
        if (mysql_options(&g_ctx.mysql, MYSQL_SET_CHARSET_NAME, "utf8") != 0) {
            cerr << "MySQL setting fail." << endl;
        }
        // 不设 MYSQL_OPT_RECONNECT：该选项已被 MySQL 8.0 弃用（启动时会打一条
        // WARNING），而且重连本就看门狗自己做，交给库反而会撞上我们的 sql_lock。

        // 客户端传来的 usr/psw 仅作为查询条件，绝不能当作 MySQL 登录凭据：
        // 否则任何远程调用者都能拿它探测数据库账号，
        // 且一旦有人以 root 登录成功，全局单例连接身份就被污染（后续查询都以该身份执行）。
        const char* sql_psw = get_sql_password();
        if (sql_psw == nullptr) {
            cerr << "MySQL password not configured, please set MYAUTO_MYSQL_PSW." << endl;
        } else if (mysql_real_connect(&g_ctx.mysql, SQL_HOST, SQL_USER, sql_psw,
            SQL_DB, SQL_PORT, NULL, 0) == NULL) {
            cerr << "Connect mysql fail: " << mysql_error(&g_ctx.mysql)
                << "!" << endl;
        } else {
            g_ctx.connected = true;
        }
        g_ctx.ready = true;

        // 启动重连监控线程（只在这里起一次）
        if (pthread_create(&g_ctx.watch_thread, NULL, watch_connect, NULL) == 0) {
            g_ctx.watch_running = true;
        } else {
            cerr << "Watchdog thread create fail." << endl;
        }
    }

    // ---------- 安全拼接 SQL（防注入） ----------
    // 调用方必须已持有 sql_lock：mysql_real_escape_string 依赖连接状态
    bool build_sql(char* buf, size_t buf_size, const char* table,
        const char* usr, const char* psw)
    {
        // 未建立连接时 mysql_real_escape_string 无法安全工作
        if (!g_ctx.connected) {
            cerr << "[DB] not connected, skip building SQL." << endl;
            return false;
        }
        // 转义用户输入（转义结果最长为输入的 2 倍，故输入限半缓冲）
        char esc_usr[128] = { 0 };
        char esc_psw[128] = { 0 };
        size_t usr_len = strnlen(usr, sizeof(esc_usr) / 2 + 1);
        size_t psw_len = strnlen(psw, sizeof(esc_psw) / 2 + 1);
        if (usr_len > sizeof(esc_usr) / 2 || psw_len > sizeof(esc_psw) / 2) {
            cerr << "[SEC] user or password too long" << endl;
            return false;
        }
        mysql_real_escape_string(&g_ctx.mysql, esc_usr, usr, usr_len);
        mysql_real_escape_string(&g_ctx.mysql, esc_psw, psw, psw_len);

        // 必须同时匹配 user 与 psw：只按 psw 匹配会让口令碰撞返回他人资料
        return snprintf(buf, buf_size,
            "SELECT email,tell FROM %s WHERE `user`='%s' AND `psw`='%s'",
            table, esc_usr, esc_psw) < (int)buf_size;
    }

    // ---------- 单次查询（持有锁期间完成，失败返回 -1 表示可重试） ----------
    int query_once(struct queryParam& param, const string& table)
    {
        pthread_mutex_lock(&g_ctx.sql_lock);

        char sql[MAX_SQL_LEN] = { 0 };
        if (!build_sql(sql, sizeof(sql), table.c_str(),
            param.user.acc ? param.user.acc : "",
            param.user.psw ? param.user.psw : "")) {
            pthread_mutex_unlock(&g_ctx.sql_lock);
            return -1;
        }
        g_ctx.call_cnt++;
#ifdef NS_DBG
        // 完整 SQL 含转义后的口令，仅在调试构建下打印
        cout << "SQL(" << g_ctx.call_cnt << "):[\033[34m" << sql << "\033[0m]" << endl;
#endif

        if (mysql_query(&g_ctx.mysql, sql) != 0) {
            cerr << "Query database fail: " << mysql_error(&g_ctx.mysql) << "!" << endl;
            pthread_mutex_unlock(&g_ctx.sql_lock);
            return -1;
        }

        MYSQL_RES* res = mysql_store_result(&g_ctx.mysql);
        if (res == nullptr) {
            pthread_mutex_unlock(&g_ctx.sql_lock);
            return -2;
        }

        memset(&param.msg, 0, sizeof(USR_MSG));
        unsigned int field_cnt = mysql_num_fields(res);
        for (unsigned int i = 0; i < field_cnt; i++) {
            mysql_fetch_field_direct(res, i);
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        int col = 0;
        while (row != nullptr) {
            if (col == 0 && row[0] != nullptr)
                memcpy(param.msg.email, row[0], sizeof(param.msg.email) - 1);
            else if (col == 1 && row[1] != nullptr)
                memcpy(param.msg.tell, row[1], sizeof(param.msg.tell) - 1);
            col++;
            row = mysql_fetch_row(res);
            param.msg.flag = true;
        }
        mysql_free_result(res);
        pthread_mutex_unlock(&g_ctx.sql_lock);

        return param.msg.flag ? 0 : -3;
    }

    // ---------- 安全查询 ----------
    int get_rslt_safe(struct queryParam& param, const string& table)
    {
        // 验证表名仅包含字母数字和下划线，防止注入和非法表名
        for (char c : table) {
            if (!(isalnum((unsigned char)c) || c == '_')) {
                cerr << "Invalid table name: " << table << endl;
                return -4;
            }
        }

        // 检查表是否存在
        char chk_sql[MAX_SQL_LEN] = { 0 };
        snprintf(chk_sql, sizeof(chk_sql), "SHOW TABLES LIKE '%s'", table.c_str());
        pthread_mutex_lock(&g_ctx.sql_lock);
        if (mysql_query(&g_ctx.mysql, chk_sql) != 0) {
            cerr << "Check table failed: " << mysql_error(&g_ctx.mysql) << "!" << endl;
            pthread_mutex_unlock(&g_ctx.sql_lock);
            return -1;
        }
        MYSQL_RES* chk_res = mysql_store_result(&g_ctx.mysql);
        if (chk_res == nullptr) {
            pthread_mutex_unlock(&g_ctx.sql_lock);
            return -1;
        }
        bool exists = (mysql_num_rows(chk_res) > 0);
        mysql_free_result(chk_res);
        pthread_mutex_unlock(&g_ctx.sql_lock);
        if (!exists) {
            cerr << "Table does not exist: " << table << endl;
            return -5;
        }

        // 首次失败很可能是连接刚断，等看门狗重连后重试一次（循环代替递归）
        for (int attempt = 0; attempt <= MAX_RETRY; attempt++) {
            int ret = query_once(param, table);
            if (ret != -1 || attempt == MAX_RETRY) {
                return ret;
            }
            cout << "[INFO] query went failure, waiting for retry..." << endl;
            sleep(3);
        }
        return -1;
    }

}  // anonymous namespace

int sqlQuery(struct queryParam& param)
{
    // 并发首调用只会初始化一次，且后到的调用者会等初始化完成
    pthread_once(&g_once, db_init_once);

#ifdef NS_DBG
    // 明文口令只在调试构建下打印
    cout << "[ACC]:" << (param.user.acc ? param.user.acc : "(null)")
        << "\t[(hash)]:" << (param.user.psw ? param.user.psw : "(null)")
        << endl;
#endif

    param.msg.flag = false;
    static const string table = "glkline";
    return get_rslt_safe(param, table);
}

void sqlClose()
{
    // 未初始化过则无需清理（pthread_once 也可能尚未触发）
    if (!g_ctx.ready) {
        return;
    }
    // 先停看门狗，再关连接，避免 ping 打到已关闭的 MYSQL 上
    g_ctx.stop.store(true);
    if (g_ctx.watch_running) {
        pthread_join(g_ctx.watch_thread, NULL);
        g_ctx.watch_running = false;
    }
    pthread_mutex_lock(&g_ctx.sql_lock);
    mysql_close(&g_ctx.mysql);
    g_ctx.connected = false;
    pthread_mutex_unlock(&g_ctx.sql_lock);

    mysql_library_end();
    pthread_mutex_destroy(&g_ctx.sql_lock);
    g_ctx.ready = false;
}
