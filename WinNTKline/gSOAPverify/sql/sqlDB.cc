#include "sqlDB.h"
using namespace std;

int it;
int num;
int cnt = 0;
char sql[64];
MYSQL mysql;
int port = 3306;
pthread_t tid;
char host[] = "localhost";
char user[] = "root";
char psw123[] = "psw123";
char db[] = "custominfo";
string table = "t_info";
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
			mysql_real_connect(&mysql, host, user, psw123, db, port, NULL, 0);
		}
		usleep(10000000);
	}
	return lp;
}

int sqlDB(int type, char* acc, char* psw, DBinfo* info)
{
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
	printf("%s[%d]--[ACC]:%s\t[(hash)]:%s\n", LL, type, acc, psw);
	sprintf(sql, "SELECT * FROM %s", table.c_str());
	if (cnt == 0)
	{
		pthread_create(&tid, NULL, test_connect, &it);
		cnt++;
	}
	if (0 != mysql_query(&mysql, sql))
	{
		mysql_close(&mysql);
		cout << LL << "Query database fail." << endl;
		return -1;
	}
	else if (type == 0)
	{
		RES = mysql_store_result(&mysql);
		num = (int)mysql_num_rows(RES);
		fieldcount = mysql_num_fields(RES);
		for (unsigned int i = 0; i<fieldcount; i++)
		{
			field = mysql_fetch_field_direct(RES, i);
		}
		fetch = mysql_fetch_row(RES);
		while (NULL != fetch)
		{
			for (int i = 0; i < (int)fieldcount; i++) {
				if (i == 5)
					sprintf(info->tele, "%s", fetch[i]);
				cout << *info->tele << "\t";
			}
			fetch = mysql_fetch_row(RES);
			cout << endl;
		}
		mysql_free_result(RES);
	}
	else {
		cout << LL << "SELECT element fail." << endl;
		mysql_close(&mysql);
		mysql_server_end();
		return -1;
	}
	return 0;
}
