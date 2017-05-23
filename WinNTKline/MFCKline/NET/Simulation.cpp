#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <windows.h>
#include <time.h>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <cmath>
#include <io.h> 
#include "ChannelCollector.h"

using namespace std;
#pragma warning(disable : 4996)

bool testfile = false;
struct st_TThostFtdc::st_MdData stMDATA; ;

void sim_test()
{
	void Simulation_onefile(char  *pathstt);

	_finddata_t file;
	long lf;
	int n = 0;

	//输入文件夹路�? 
	if ((lf = _findfirst("..\\Bin\\testdata\\*.*", &file)) == -1)
		cout << "Not Found!" << endl;
	else
	{
		while (_findnext(lf, &file) == 0)
		{
			cout << file.name << endl;
			Sleep(1000);

			if (n>0)
			{
				cout << "准备测试�? << n << "个文件！" << endl;

				string str = file.name;
				string str0 = "";
				string str2 = ".txt";

				size_t iPos = str.find(".");
				str0 = str.substr(iPos, 4);

				if (str0 == str2.c_str())
				{
					testfile = true;
				}
				else
				{
					testfile = false;
				}
				char *str1 = file.name;
				Simulation_onefile(str1);
			}
			n = n + 1;
		}
	}
	_findclose(lf);
}

void _record0(char *txt)
{
	ofstream o_file(stMDATA.LogFilePaths, ios::app);
	o_file << txt << endl;
	o_file.close();
}

void Sniffer() {};
void Istrading() {};

void Simulation_onefile(char  *pathstt)
{
	void _record0(char *txt);

	char Readfilepaths[50] = "";
	strcpy(Readfilepaths, "./testdata/");
	strcat(Readfilepaths, pathstt);
	//cout << Readfilepaths << endl;	//打印整行数据

	ifstream fin(Readfilepaths, std::ios::in);

	string str = pathstt;
	string str0 = "";
	size_t iPos = str.find("_");
	str0 = str.substr(0, iPos);

	strcpy(stMDATA.LogFilePaths, "./Simulation/Simulation_");
	strcat(stMDATA.LogFilePaths, str0.c_str());
	strcat(stMDATA.LogFilePaths, ".txt");

	Sleep(3000);
	char line[1024] = { 0 };
	vector < double > data(16);

	_record0("开始测试！！！");
	int t0 = 0;

	while (fin.getline(line, sizeof(line)))
	{
		std::stringstream word(line);
		//cout << line << endl;	//打印整行数据
		//Sleep(100);

		t0 = t0 + 1;

		for (int i = 0; i < 20; i++)
		{
			if (stMDATA.InstrumentID_n[i] == str0 && testfile)
			{
				stMDATA.InstrumentID_name = stMDATA.InstrumentID_n[i];
				stMDATA.tick_data[i][0] = 1;//设置标志�?
				stMDATA.ReceiveTick[i] = true;

				for (int j = 0; j < 10; j++)
				{
					word >> data[j];
					//cout << "Configuration:" << data[j] << endl;	//打印某个数据 
				}

				stMDATA.Q_BarTime_1n[i] = (int(data[1] * 100)) * 60 * 60 + (int((data[1] * 100 - int(data[1] * 100)) * 100)) * 60 + (int((data[1] * 10000 - int(data[1] * 10000)) * 100));

				stMDATA.tick_data[i][1] = data[0];
				stMDATA.tick_data[i][2] = data[1];

				if ((stMDATA.tick_data[i][2]>0.0856 && stMDATA.tick_data[i][2]<0.0900 && i <= 13) || (stMDATA.tick_data[i][2]>0.2056 && stMDATA.tick_data[i][2]<0.2100 && i>13 && i <= 17) || (stMDATA.tick_data[i][2]>0.0913 && stMDATA.tick_data[i][2]<0.0915 && i>17))
				{
					stMDATA.Day_open[i][0] = data[3];
					stMDATA.Day_CloseProfit[i] = 0;
					stMDATA.Day_CloseProfitA[i] = 0;
					stMDATA.Day_CloseProfitB[i] = 0;

				}
				for (int j = 59; j > 0; j--)
				{
					stMDATA.tick_AskPrice1[i][j] = stMDATA.tick_AskPrice1[i][j - 1];
					stMDATA.tick_BidPrice1[i][j] = stMDATA.tick_BidPrice1[i][j - 1];
					stMDATA.tick_AskVolume1[i][j] = stMDATA.tick_AskVolume1[i][j - 1];
					stMDATA.tick_BidVolume1[i][j] = stMDATA.tick_BidVolume1[i][j - 1];
					stMDATA.tick_Volume[i][j] = stMDATA.tick_Volume[i][j - 1];
					stMDATA.tick_OpenInterest[i][j] = stMDATA.tick_OpenInterest[i][j - 1];
				}
				stMDATA.tick_AskPrice1[i][0] = data[3];
				stMDATA.tick_BidPrice1[i][0] = data[5];
				stMDATA.tick_AskVolume1[i][0] = data[4];
				stMDATA.tick_BidVolume1[i][0] = data[6];
				stMDATA.tick_Volume[i][0] = data[8];
				stMDATA.tick_OpenInterest[i][0] = data[9];
				//cout << "Configuration:" << tick_data[i][2] << endl;	//打印某个数据
				stMDATA.ReceiveTick[i] = false;
				//}
				Sniffer();
				Istrading();
				Sniffer();
				Istrading();			//重复是为了和实际一致，1个tick收到后策略可能被多次运行�?
									//Sleep(100);
									//cout << "Configuration:2_" << setprecision(9) <<GetLocalTimeMs1() << endl;	//打印某个数据
			}
		}
	}
	fin.clear();
	fin.close();
}
