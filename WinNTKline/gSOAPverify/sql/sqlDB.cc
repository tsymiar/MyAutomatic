#include "sqlDB.h"

using namespace std;

	int num;	
	char sql[64];
	MYSQL mysql;
	int port=3306;
	char host[]="localhost";
	char user[]="root";
	char psw123[]="psw123";
	char db[]="" /*"custominfo"*/;
	string table="uinfo";
	char LL[] = ">>>";
	MYSQL_RES *RES = nullptr;
	MYSQL_FIELD *field = NULL;
	unsigned int fieldcount;
	MYSQL_ROW fetch = NULL;

void dptb()
{
	string sql = "DROP TABLE " + table;

	if (0 != mysql_query(&mysql, sql.c_str()))
	{
		mysql_close(&mysql);
		cout << LL << "DROP TABLE fail." << endl;
	}
	mysql_free_result(RES);
	mysql_close(&mysql);
	mysql_server_end();
}

int sqlDB(int type, char* acc, char* psw, char* out[])
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
	printf("%s[%d]--[ACC]:%s\t[(hash)]:%s\n",LL,type,acc,psw);
	sprintf(sql,"SELECT %s FROM %s",acc,table.c_str());

	if (0 != mysql_query(&mysql, sql))
	{
		cout << LL << "Query database fail." << endl;
		dptb();
		return -1;
	} else if(type==0)
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
				cout << fetch[i] << "\t\t";
			}
			fetch = mysql_fetch_row(RES);
		}
		return 0;
	}
	return 0;
}
