#include "sqldb.h"

using namespace std;

int rows;
string sql;
MYSQL mysql;
MYSQL_RES *result = nullptr;
MYSQL_FIELD *field = NULL;
unsigned int fieldcount;
MYSQL_ROW fetch = NULL;

int sqldb() {

	if (0 != mysql_library_init(0, NULL, NULL))
		cout << "lib init fail." << endl;
	if (NULL == mysql_init(&mysql))
		cout << "mysql init fail." << endl;
	if (0 != mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "GBK"))
		cout << "mysql setting fail." << endl;
	if (NULL == mysql_real_connect(&mysql, "localhost", "mysql", "psw123", "custominfo", 3306, NULL, 0))
		cout << "mysql connect fail." << endl;
	if (0 != mysql_query(&mysql, sql.c_str()))
	{
		mysql_close(&mysql);
		cout << "mysql query fail." << endl;
	}
	sql = "select id,Name,AGE,email,sex from user";
	if (0 == mysql_query(&mysql, sql.c_str()))
	{
		result = mysql_store_result(&mysql);
		rows = (int)mysql_num_rows(result);
		fieldcount = mysql_num_fields(result);
		for (unsigned int i = 0; i<fieldcount; i++)
		{
			field = mysql_fetch_field_direct(result, i);
		}
		fetch = mysql_fetch_row(result);
		while (NULL != fetch)
		{
			for (int i = 0; i < (int)fieldcount; i++) {
				cout << fetch[i] << "\t\t";
			}
			fetch = mysql_fetch_row(result);
		}
	}
	else {
		mysql_close(&mysql);
		cout << "select data fail." << endl;
	}
	sql = "drop table user";
	if (0 != mysql_query(&mysql, sql.c_str()))
	{
		mysql_close(&mysql);
		cout << "drop table fail." << endl;
	}
	mysql_free_result(result);
	mysql_close(&mysql);
	mysql_server_end();
	return 0;
}