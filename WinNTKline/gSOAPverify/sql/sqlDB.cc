#include "sqlDB.h"

using namespace std;

int it;
int num;
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
char LL[] = ">>>";
MYSQL_RES *RES = nullptr;
MYSQL_FIELD *field = NULL;
unsigned int fieldcount;
MYSQL_ROW fetch = NULL;

void* test_connect(void* lp)
{
	while (1)
	{
		if (mysql_ping(&mysql) != 0) {
			if (mysql_real_connect(&mysql, host, user, psw123, db, port, NULL, 0) == 0)
				cout << LL << "Connect mysql fail." << endl;
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
		RES = mysql_store_result(&mysql);
		num = (int)mysql_num_rows(RES);
		fieldcount = mysql_num_fields(RES);
		for (unsigned int i = 0; i < fieldcount; i++)
		{
			field = mysql_fetch_field_direct(RES, i);
		}
		fetch = mysql_fetch_row(RES);
		while (NULL != fetch)
		{
			for (int i = 0; i < (int)fieldcount; i++) {
				switch (i)
				{
				case 3:
					memcpy(info->email, fetch[i], 32);
					break;
				case 4:
					memcpy(info->tell, fetch[i], 14);
					break;
				default:break;
				}
			}
			fetch = mysql_fetch_row(RES);
			info->flg = 1;
		}
		mysql_free_result(RES);
		pthread_mutex_unlock(&sql_lock);
	}
	else {
		cout << LL << "SELECT element fail." << endl;
		mysql_close(&mysql);
		mysql_server_end();
		pthread_mutex_unlock(&sql_lock);
		return -1;
	}
	return 0;
}
