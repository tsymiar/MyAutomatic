#include "sqlDB.h"

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
char psw123[] = "psw123";
char db[] = "custominfo";
string table = "myweb";
char LL[] = "\033[K>>>";
MYSQL_RES *RES = nullptr;
MYSQL_FIELD *field = NULL;
unsigned int rownum;
unsigned int fieldnum;
MYSQL_ROW fetch = NULL;
bool get_rslt_new(struct DBinfo* info);
bool get_rslt_raw(struct DBinfo* info)
{
	RES = mysql_store_result(&mysql);
	rownum = (int)mysql_num_rows(RES);
	fieldnum = mysql_num_fields(RES);
	info->msg = (st_usr_msg*)malloc(sizeof(st_usr_msg));
	memset(info->msg, 0, sizeof(st_usr_msg));
	for (unsigned int i = 0; i < fieldnum; i++)
	{
		field = mysql_fetch_field_direct(RES, i);
	}
	fetch = mysql_fetch_row(RES);
	while (NULL != fetch)
	{
		for (int i = 0; i < (int)fieldnum; i++) {
			switch (i)
			{
			case 3:
				memcpy(info->msg->email, fetch[i], 32);
				break;
			case 4:
				memcpy(info->msg->tell, fetch[i], 14);
				break;
			default:break;
			}
		}
		fetch = mysql_fetch_row(RES);
		info->flg = true;
	}
	mysql_free_result(RES);
	return info->flg;
}

void* test_connect(void* lp)
{
	while (1)
	{
		if (mysql_ping(&mysql) != 0) {
			if ((mysql_real_connect(&mysql, host, user, psw123, db, port, NULL, 0) == 0) && (j < 9))
			{
				cout << LL << "Connect mysql fail." << endl;
				j++;
			}
		}
		usleep(10000000);
	}
	return lp;
}

int sqlDB(int type, char* acc, char* psw, struct DBinfo* info)
{
	if (cnt == 0)
	{
		pthread_mutex_init(&sql_lock, NULL);
		if (0 != mysql_library_init(0, NULL, NULL))
			cout << LL << "lib init fail." << endl;
		if (NULL == mysql_init(&mysql))
			cout << LL << "MySQL init fail." << endl;
		if (0 != mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "GBK"))
			cout << LL << "MySQL setting fail." << endl;
		if (NULL == mysql_real_connect(&mysql, host, user, psw123, db, port, NULL, 0))
			cout << LL << "Connect mysql fail." << endl;
		//int Select(char* table, char** RES, const char* factor,...)
		//"SELECT Name,AGE,sex,email FROM ...";
		pthread_create(&tid, NULL, test_connect, &it);
		cnt++;
	}
	cout << LL << "[" << type << "]--[ACC]:" << acc << "\t[(hash)]:" << psw << endl;
#ifdef FIX
	st_raw raw{ type, acc, psw };
	info->raw = raw;
	get_rslt_new(info);
#else
	sprintf(sql, "SELECT * FROM %s WHERE psw='%s'", table.c_str(), psw);
	cout << LL << "SQL:[\033[34m" << sql << "\033[0m]" << endl;
	pthread_mutex_lock(&sql_lock);
	if (0 != mysql_query(&mysql, sql))
	{
		cout << LL << "Query database fail." << endl;
		mysql_close(&mysql);
		pthread_mutex_unlock(&sql_lock);
		return -1;
	}
	else if (type == 0)
	{
		get_rslt_raw(info);
		pthread_mutex_unlock(&sql_lock);
	}
	else {
		cout << LL << "SELECT element fail." << endl;
		mysql_close(&mysql);
		mysql_server_end();
		pthread_mutex_unlock(&sql_lock);
		return -1;
	}
#endif // FIX
	return 0;
}

void sql_close()
{
	mysql_close(&mysql);
}

bool get_rslt_new(DBinfo * info)
{
	sprintf(sql, "SELECT email,tell FROM %s WHERE psw='%s'" /*AND user='%s'"*/, table.c_str(), info->raw.psw/*, info->raw.acc*/);
	cout << LL << "SQL:[\033[34m" << sql << "\033[0m]" << endl;
	pthread_mutex_lock(&sql_lock);
	if (0 != mysql_query(&mysql, sql))
	{
		cout << LL << "Query database fail." << endl;
		mysql_close(&mysql);
		pthread_mutex_unlock(&sql_lock);
		return -1;
	}
	else if (info->raw.type == 0)
	{
		RES = mysql_store_result(&mysql);
		info->msg = (st_usr_msg*)malloc(sizeof(st_usr_msg));
		memset(info->msg, 0, sizeof(st_usr_msg));
		for (unsigned int i = 0; i < mysql_num_fields(RES); i++)
			mysql_fetch_field_direct(RES, i);
		int j = 0;
		fetch = mysql_fetch_row(RES);
		while (NULL != fetch)
		{
			if (j == 0)
				memcpy(info->msg->email, fetch[0], 32);
			else if (j == 1)
				memcpy(info->msg->tell, fetch[1], 14);
			j++;
			fetch = mysql_fetch_row(RES);
			info->flg = true;
		}
		mysql_free_result(RES);
		pthread_mutex_unlock(&sql_lock);
		return info->flg;
	}
	else {
		cout << LL << "SELECT element fail." << endl;
		mysql_close(&mysql);
		mysql_server_end();
		pthread_mutex_unlock(&sql_lock);
		return -1;
	}
}
