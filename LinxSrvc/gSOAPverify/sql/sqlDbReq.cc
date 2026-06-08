#include "sqlDbReq.h"

using namespace std;

namespace {

    // MySQL 连接配置常量 (放在 struct 外部避免 AARCH64 static constexpr 链接问题)
    constexpr int  SQL_PORT         = 3306;
    constexpr int  MAX_RECONNECT    = 9;
    const     char SQL_HOST[]       = "localhost";
    const     char SQL_USER[]       = "root";
    const     char SQL_DB[]         = "myautomatic";

    struct DbContext {
        MYSQL         mysql;
        int           call_cnt   = 0;
        int           reconnect  = 0;
        bool          connected  = false;
        pthread_t     watch_thread;
        pthread_mutex_t sql_lock;
    } g_ctx;

    // 注意: 密码从环境变量 MYAUTO_MYSQL_PSW 读取
    // 未设置报错并跳过连接，避免硬编码密码和泄露风险
    const char* get_sql_password()
    {
        const char* env = getenv("MYAUTO_MYSQL_PSW");
        if (env == nullptr || env[0] == '\0') {
            cerr << "Environment variable MYAUTO_MYSQL_PSW is not set." << endl;
            return nullptr;
        }
        return env;
    }

    // ---------- 连接监控线程 ----------
    void* watch_connect(void*)
    {
        char val = 1;
        mysql_options(&g_ctx.mysql, MYSQL_OPT_RECONNECT, &val);
        while (true) {
            if (mysql_ping(&g_ctx.mysql) != 0) {
                const char* sql_psw = get_sql_password();
                if (sql_psw == nullptr) {
                    cerr << "Reconnect skipped: MySQL password not configured." << endl;
                } else if (!mysql_real_connect(&g_ctx.mysql,
                    SQL_HOST, SQL_USER,
                    sql_psw,
                    SQL_DB, SQL_PORT,
                    NULL, 0)
                    && g_ctx.reconnect < MAX_RECONNECT) {
                    cerr << "Connect mysql fail: " << mysql_error(&g_ctx.mysql)
                        << "!" << endl;
                    g_ctx.reconnect++;
                }
            }
            sleep(3);
        }
        return nullptr;
    }

    // ---------- 安全拼接 SQL（防注入） ----------
    bool build_sql(char* buf, size_t buf_size, const char* table,
        const char* psw)
    {
        // 转义用户输入的密码
        char escaped[128] = { 0 };
        size_t psw_len = strlen(psw);
        if (psw_len > sizeof(escaped) / 2) {
            cerr << "[SEC] password too long" << endl;
            return false;
        }
        mysql_real_escape_string(&g_ctx.mysql, escaped, psw, psw_len);

        return snprintf(buf, buf_size,
            "SELECT email,tell FROM %s WHERE `psw`='%s'",
            table, escaped) < (int)buf_size;
    }

    // ---------- 安全查询 (FIX 路径) ----------
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
        char chk_sql[MAX_SQL_LEN] = {0};
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

        char sql[MAX_SQL_LEN] = { 0 };
        if (!build_sql(sql, sizeof(sql), table.c_str(), param.user.psw)) {
            return -1;
        }
        cout << "SQL(" << g_ctx.call_cnt << "):[\033[34m" << sql << "\033[0m]" << endl;

        pthread_mutex_lock(&g_ctx.sql_lock);
        g_ctx.call_cnt++;

        if (mysql_query(&g_ctx.mysql, sql) != 0) {
            if (g_ctx.call_cnt == 1) {
                pthread_mutex_unlock(&g_ctx.sql_lock);
                cout << "[INFO] first time went failure, waiting for retry..." << endl;
                sleep(3);
                return get_rslt_safe(param, table);
            }
            cerr << "Query database fail: " << mysql_error(&g_ctx.mysql) << "!" << endl;
            pthread_mutex_unlock(&g_ctx.sql_lock);
            return -1;
        }

        if (!param.msg.flag) {
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

        cerr << "SELECT element fail: " << mysql_error(&g_ctx.mysql) << "!" << endl;
        mysql_close(&g_ctx.mysql);
        pthread_mutex_unlock(&g_ctx.sql_lock);
        return -3;
    }

#ifndef FIX
    // ---------- 旧版不安全查询 (仅非 FIX 路径使用) ----------
    bool get_rslt_raw(struct queryParam& param, const string& table,
        const char* sql_buf, size_t buf_size)
    {
        char sql[MAX_SQL_LEN];
        snprintf(sql, sizeof(sql), "SELECT * FROM %s WHERE psw='%s'",
            table.c_str(), param.user.psw);
        cout << "SQL(" << g_ctx.call_cnt << "):[\033[34m" << sql << "\033[0m]" << endl;

        MYSQL_RES* res = mysql_store_result(&g_ctx.mysql);
        if (res == nullptr) return false;

        mysql_num_rows(res);
        unsigned int field_cnt = mysql_num_fields(res);

        memset(&param.msg, 0, sizeof(USR_MSG));
        for (unsigned int i = 0; i < field_cnt; i++) {
            mysql_fetch_field_direct(res, i);
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        while (row != nullptr) {
            for (unsigned int i = 0; i < field_cnt; i++) {
                switch (i) {
                case 3:
                    if (row[i] != nullptr)
                        memcpy(param.msg.email, row[i], sizeof(param.msg.email) - 1);
                    break;
                case 4:
                    if (row[i] != nullptr)
                        memcpy(param.msg.tell, row[i], sizeof(param.msg.tell) - 1);
                    break;
                default: break;
                }
            }
            row = mysql_fetch_row(res);
            param.msg.flag = true;
        }
        mysql_free_result(res);
        return param.msg.flag;
    }
#endif

}  // anonymous namespace

int sqlQuery(struct queryParam& param, bool flag)
{
    // 首次调用时初始化连接
    if (g_ctx.call_cnt == 0) {
        pthread_mutex_init(&g_ctx.sql_lock, NULL);

        if (mysql_library_init(0, NULL, NULL) != 0)
            cerr << "lib init fail." << endl;
        if (mysql_init(&g_ctx.mysql) == NULL)
            cerr << "MySQL init fail." << endl;
        if (mysql_options(&g_ctx.mysql, MYSQL_SET_CHARSET_NAME, "utf8") != 0)
            cerr << "MySQL setting fail." << endl;

        // 用 SOAP 传来的用户名/密码尝试连接 MySQL
        const char* auth_user = (param.user.acc && param.user.acc[0])
            ? param.user.acc : SQL_USER;
        const char* auth_psw = (param.user.psw && param.user.psw[0])
            ? param.user.psw : get_sql_password();

        if (auth_psw == nullptr) {
            cerr << "MySQL password not configured and no SOAP password provided." << endl;
        } else if (mysql_real_connect(&g_ctx.mysql, SQL_HOST,
            auth_user, auth_psw,
            SQL_DB, SQL_PORT, NULL, 0) == NULL) {
            cerr << "Connect mysql fail: " << mysql_error(&g_ctx.mysql)
            << "!" << endl;
        } else {
            g_ctx.connected = true;
        }

        // 启动重连监控线程
        pthread_create(&g_ctx.watch_thread, NULL, watch_connect, NULL);
    }

    cout << "[" << (flag ? "true" : "false") << "]--[ACC]:"
        << (param.user.acc ? param.user.acc : "(null)")
        << "\t[(hash)]:" << (param.user.psw ? param.user.psw : "(null)")
        << endl;

    int ret = -1;
    static const string table = "glkline";

#ifdef FIX
    param.msg.flag = flag;
    ret = get_rslt_safe(param, table);
#else
    if (flag) {
        cout << "Element flag invalid." << endl;
        mysql_close(&g_ctx.mysql);
        mysql_server_end();
    } else {
        pthread_mutex_lock(&g_ctx.sql_lock);
        char sql_buf[MAX_SQL_LEN];
        if (get_rslt_raw(param, table, sql_buf, sizeof(sql_buf)))
            ret = 0;
        g_ctx.call_cnt++;
        pthread_mutex_unlock(&g_ctx.sql_lock);
    }
#endif

    return ret;
}

void sqlClose()
{
    pthread_mutex_lock(&g_ctx.sql_lock);
    mysql_close(&g_ctx.mysql);
    pthread_mutex_unlock(&g_ctx.sql_lock);
    pthread_mutex_destroy(&g_ctx.sql_lock);
}
