#include "sqlDbReq.h"

using namespace std;

int it;
int j = 0;
int cnt = 0;
char sql[64];
MYSQL mysql;
int port = 3306;
pthread_t tid;
pthread_mutex_t sql_lock;
char host[] = "localhost";
char user[] = "root";
char psw[] = "Psw123$";
char db[] = "myautomatic";
string table = "glkline";
char LL[] = "\033[K>>>";
MYSQL_RES* SQL_RES = nullptr;
MYSQL_FIELD* field = NULL;
unsigned int rownum;
unsigned int fieldnum;
MYSQL_ROW fetch = NULL;
int get_rslt_new(struct queryParam* param);

bool get_rslt_raw(struct queryParam* param)
{
    if (param == nullptr) {
        return false;
    }
    sprintf(sql, "SELECT * FROM %s WHERE psw='%s'", table.c_str(), psw);
    cout << LL << "SQL(" << cnt << "):[\033[34m" << sql << "\033[0m]" << endl;
    SQL_RES = mysql_store_result(&mysql);
    rownum = (int)mysql_num_rows(SQL_RES);
    fieldnum = mysql_num_fields(SQL_RES);
    memset(&param->msg, 0, sizeof(USR_MSG));
    for (unsigned int i = 0; i < fieldnum; i++) {
        field = mysql_fetch_field_direct(SQL_RES, i);
    }
    fetch = mysql_fetch_row(SQL_RES);
    while (NULL != fetch) {
        for (int i = 0; i < (int)fieldnum; i++) {
            switch (i) {
            case 3:
                memcpy(param->msg.email, fetch[i], 32);
                break;
            case 4:
                memcpy(param->msg.tell, fetch[i], 14);
                break;
            default:break;
            }
        }
        fetch = mysql_fetch_row(SQL_RES);
        param->msg.flag = true;
    }
    mysql_free_result(SQL_RES);
    return param->msg.flag;
}

void* test_connect(void* lp)
{
    char val = 1;
    mysql_options(&mysql, MYSQL_OPT_RECONNECT, (char*)&val);
    while (1) {
        if (mysql_ping(&mysql) != 0) {
            if ((mysql_real_connect(&mysql, host, user, psw, db, port, NULL, 0) == 0) && (j < 9)) {
                cout << LL << "Connect mysql fail: " << mysql_error(&mysql) << "!" << endl;
                j++;
            }
        }
        sleep(3);
    }
    return lp;
}

int sqlQuery(struct queryParam param, bool flag)
{
    if (cnt == 0) {
        pthread_mutex_init(&sql_lock, NULL);
        if (0 != mysql_library_init(0, NULL, NULL))
            cout << LL << "lib init fail." << endl;
        if (NULL == mysql_init(&mysql))
            cout << LL << "MySQL init fail." << endl;
        if (0 != mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf8"))
            cout << LL << "MySQL setting fail." << endl;
        if (NULL == mysql_real_connect(&mysql, host, param.user.acc, param.user.psw, db, port, NULL, 0))
            cout << LL << "Connect mysql fail: " << mysql_error(&mysql) << "!" << endl;
        //int Select(char* table, char** SQL_RES, const char* factor,...)
        //"SELECT Name,AGE,sex,email FROM ...";
        pthread_create(&tid, NULL, test_connect, &it);
    }
    cout << LL << "[" << flag << "]--[ACC]:" << param.user.acc << "\t[(hash)]:" << param.user.psw << endl;
    int ret = -1;
#ifdef FIX
    param.msg.flag = flag;
    ret = get_rslt_new(&param);
#else
    if (flag) {
        cout << LL << "Element flag invalid." << endl;
        mysql_close(&mysql);
        mysql_server_end();
    } else {
        pthread_mutex_lock(&sql_lock);
        if (get_rslt_raw(&param))
            ret = 0;
        cnt++;
        pthread_mutex_unlock(&sql_lock);
    }
#endif // FIX
    return ret;
}

int get_rslt_new(queryParam* param)
{
    sprintf(sql, "SELECT email,tell FROM %s WHERE `psw`='%s'" /*AND user='%s'"*/, table.c_str(), param->user.psw/*, param->raw.acc*/);
    cout << LL << "SQL(" << cnt << "):[\033[34m" << sql << "\033[0m]" << endl;
    pthread_mutex_lock(&sql_lock);
    cnt++;
    if (0 != mysql_query(&mysql, sql)) {
        if (cnt == 1) {
            pthread_mutex_unlock(&sql_lock);
            cout << LL << "[INFO] first time went failure, waiting for retring..." << endl;
            sleep(3);
            return get_rslt_new(param);
        }
        cout << LL << "Query database fail: " << mysql_error(&mysql) << "!" << endl;
        mysql_close(&mysql);
        pthread_mutex_unlock(&sql_lock);
        return -1;
    } else if (!param->msg.flag) {
        SQL_RES = mysql_store_result(&mysql);
        memset(&param->msg, 0, sizeof(USR_MSG));
        for (unsigned int i = 0; i < mysql_num_fields(SQL_RES); i++)
            mysql_fetch_field_direct(SQL_RES, i);
        int j = 0;
        fetch = mysql_fetch_row(SQL_RES);
        while (NULL != fetch) {
            if (j == 0)
                memcpy(param->msg.email, fetch[0], 32);
            else if (j == 1)
                memcpy(param->msg.tell, fetch[1], 14);
            j++;
            fetch = mysql_fetch_row(SQL_RES);
            param->msg.flag = true;
        }
        mysql_free_result(SQL_RES);
        pthread_mutex_unlock(&sql_lock);
        return (param->msg.flag ? 0 : -2);
    } else {
        cout << LL << "SELECT element fail: " << mysql_error(&mysql) << "!" << endl;
        mysql_close(&mysql);
        mysql_server_end();
        pthread_mutex_unlock(&sql_lock);
        return -3;
    }
    return 0;
}

void sqlClose()
{
    mysql_close(&mysql);
}
