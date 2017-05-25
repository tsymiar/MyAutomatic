#pragma once
//
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <windows.h>
#include <io.h> 

#include "ChannelCollector.h"

using namespace std;
#pragma warning(disable : 4996)

#define EPSILON_E4 (float)(1E-2) 
#define EPSILON_E5 (float)(1E-3)
void Simulation();
extern	int		simMode;
// 请求编号
int		iRequestID = 0;											//请求编号
char  FRONT_ADDR_1A[] = "tcp://180.168.212.51:41205";		// 前置地址1交易:实盘
char  FRONT_ADDR_1B[] = "tcp://180.168.212.51:41213";		// 前置地址1行情:实盘
// 交易时间
TThostFtdcDateExprType	TradingDay;								//交易日期

bool	JustRun = false;										//正在启动标志
bool	CloseAll= false;										//收盘标志

int		FirstVolume=0;											//前一次成交量数据

string	InstrumentID_name="";	//缓存TICK合约名称

int		Q_BarTime_1 = 0;		//缓存TICK时间戳：秒计算
double	Q_BarTime_2 = 0;		//缓存TICK时间戳：0.145500

int		Q_BarTime_1n[20]	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//合约时间戳：秒计算
int		Trade_times[20]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//开仓时间戳：秒计算

char    LogFilePaths[80]="";			//交易日志
char	TickFileWritepaths[20][80]	= { "", "", "", "", "", "", "", "", "", "" ,"", "", "", "", "", "", "", "", "", "" };	//TICK数据保存文件名次格式，合约名称_日期.txt

//											0		1			2		  3			4		 5		  6		   7		8			9		10		11		12		13		14		15		  16		17	   18		 19
char	InstrumentID_n[20][10]		= {  "i1409", "jm1409", "j1409", "rb1410","rb1501", "TA409", "l1409","ru1409", "ru1501", "jd1409", "RM409", "m1409","y1501", "p1501","ag1506","ag1412","cu1408","cu1409","IF1409","IF1407"};//交易合约

int		InstrumentID_En[20]			= { 0       ,        0,       0,        0,       0,       0,       0,       0,        0,        0,       0,       0,      0,       0,       0,       0,       0,      0,	 0,       0  };	//交易使能 =1，会实盘下单
int		InstrumentID_lots[20]		= { 1       ,        1,       1,        1,       1,       1,       1,       1,        1,        1,       1,       1,      1,       1,       1,       1,       1,      1,	 1,       1  };	//开仓量

double  InstrumentID_minmove[20]	= { 1       ,        1,       1,        1,       1,       2,       5,       5,        5,        1,       1,       1,      2,       2,       1,       1,      10,     10,   0.2,     0.2, };	//最小变动价位

double	Trade_Stopwin[20]			= { 30       ,      30,      30,       20,      20,      30,      30,     120,      120,       30,      30,      30,     30,      30,      30,      30,      30,     30,    30,      30  };	//单次开仓止赢点
double	Trade_Stoploss[20]			= { 30       ,      30,      30,       20,      20,      30,      30,     180,      180,       30,      30,      30,     30,      30,      30,      30,      30,     30,    30,      30  };	//单次开仓止损点
double	Trade_StopCloseProfit[20]	= { 30       ,      30,      30,       10,      10,      30,      30,     110,      110,       30,      30,      30,     30,      30,      30,      30,      30,     30,    30,      30  };	//>止损,限制新开


//tick数据
bool	FristTick[20]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//收到当日第一个有效TICK标记
bool	LastTick[20]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//收到当日最后一个有效TICK标记
bool	ReceiveTick[20]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//TICK数据接收标记，暂未使用

double	tick_data[20][10]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:基本信息

double	tick_AskPrice1[20][60]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
double	tick_BidPrice1[20][60]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
double	tick_AskVolume1[20][60]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
double	tick_BidVolume1[20][60]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
double	tick_Volume[20][60]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
double	tick_OpenInterest[20][60]	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据

double	Sniffer_dataA[20][10]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
double	Sniffer_dataB[20][10]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
double	Sniffer_dataC[20][10]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
double	Sniffer_dataD[20][10]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据

//----------------------
double  Day_open[20][60]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//日K线数据开 
double  Day_high[20][60]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//日K线数据高
double  Day_low[20][60]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//日K线数据低
double  Day_close[20][60]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//日K线数据收

bool	MnKlinesig[20]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//分钟K线第一个TICK标记
double  Mn_open[20][60]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//分钟K线数据开 
double  Mn_high[20][60]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//分钟K线数据高
double  Mn_low[20][60]				= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//分钟K线数据低
double  Mn_close[20][60]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//分钟K线数据收

//-----------------------
bool	SnifferSignalA[20]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//指标策略运算标记
bool	TradingSignalA[20]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//下单标记
bool	SnifferSignalB[20]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//指标策略运算标记
bool	TradingSignalB[20]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//下单标记

//基本订单数据
double	Trade_dataA[20][10]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
double	Trade_dataB[20][10]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
double	Trade_dataC[20][10]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
double	Trade_dataD[20][10]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据


//测试开平仓统计数据
double	Trade_CloseProfit[20]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//平仓盈亏，测试策略用
double	Trade_Closetimes[20]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//平仓次数，测试策略用
double	Day_CloseProfit[20]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//当日平仓收益，测试策略用
double	Day_CloseProfitA[20]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//当日平仓收益，测试策略用
double	Day_CloseProfitB[20]		= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//当日平仓收益，测试策略用
double	Day_TradeNumb[20]			= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//开仓次数统计

////////////////////////////////////////

extern int		Q_BarTime_1;		//

extern	int		Q_BarTime_1n[20];	//

extern	int		simMode;
extern	int		DebugMode;

extern char		InstrumentID_n[20][10];
extern int		InstrumentID_lots[20];
extern double	InstrumentID_minmove[20];

extern	double  Day_open[20][60];	//日K线开
extern	double  Day_high[20][60];	//日K线高
extern	double  Day_low[20][60];	//日K线低
extern	double  Day_close[20][60];	//日K线收

extern	double  Mn_open[20][60];	//分钟K线开
extern	double  Mn_high[20][60];	//分钟K线高
extern	double  Mn_low[20][60];		//分钟K线低
extern	double  Mn_close[20][60];	//分钟K线收

extern  double  BuyPrice[20];		//开仓价
extern  double  SellPrice[20];		//开仓价
extern	int		BNum[20];			//开仓次数
extern	int		SNum[20];			//开仓次数
extern	int		BNum2[20];			//开仓次数
extern	int		SNum2[20];			//开仓次数

extern double	tick_data[20][10];				//TICK基本数据
extern	double	tick_AskPrice1[20][60];			//买一价
extern	double	tick_BidPrice1[20][60];			//卖一价
extern	double	tick_AskVolume1[20][60];		//买一量
extern	double	tick_BidVolume1[20][60];		//卖一量
extern	double	tick_Volume[20][60];			//成交量
extern	double	tick_OpenInterest[20][60];		//持仓量

extern	double	Sniffer_dataA[20][10];//监测数据
extern	double	Sniffer_dataB[20][10];//监测数据
extern	double	Sniffer_dataC[20][10];//监测数据
									  //基本订单数据
extern	double	Trade_dataA[20][10];//参数
extern	double	Trade_dataB[20][10];//参数

extern double	Trade_Stopwin[20];
extern double	Trade_Stoploss[20];

extern double	Trade_Largevol[20];
extern double	Trade_Littelvol[20];
extern double	Trade_CloseProfit[20];
extern double	Trade_Closetimes[20];
extern	int		Trade_times[20];	//

extern bool		SnifferSignalA[20];
extern bool		TradingSignalA[20];

extern	double	Day_CloseProfit[20];
extern	double	Trade_StopCloseProfit[20];


extern	double	Day_BuyPos[20];
extern	double	Day_SellPos[20];

///strategy_B


extern int		Q_BarTime_1;		//

extern	int		Q_BarTime_1n[20];	//

extern	int		simMode;
extern	int		DebugMode;

extern char		InstrumentID_n[20][10];
extern int		InstrumentID_lots[20];
extern double	InstrumentID_minmove[20];

extern	double  Day_open[20][60];	//日K线开
extern	double  Day_high[20][60];	//日K线高
extern	double  Day_low[20][60];	//日K线低
extern	double  Day_close[20][60];	//日K线收

extern	double  Mn_open[20][60];	//分钟K线开
extern	double  Mn_high[20][60];	//分钟K线高
extern	double  Mn_low[20][60];		//分钟K线低
extern	double  Mn_close[20][60];	//分钟K线收

extern  double  BuyPrice[20];		//开仓价
extern  double  SellPrice[20];		//开仓价
extern	int		BNum[20];			//开仓次数
extern	int		SNum[20];			//开仓次数
extern	int		BNum2[20];			//开仓次数
extern	int		SNum2[20];			//开仓次数

extern double	tick_data[20][10];				//TICK基本数据
extern	double	tick_AskPrice1[20][60];			//买一价
extern	double	tick_BidPrice1[20][60];			//卖一价
extern	double	tick_AskVolume1[20][60];		//买一量
extern	double	tick_BidVolume1[20][60];		//卖一量
extern	double	tick_Volume[20][60];			//成交量
extern	double	tick_OpenInterest[20][60];		//持仓量

extern	double	Sniffer_dataA[20][10];//监测数据
extern	double	Sniffer_dataB[20][10];//监测数据
extern	double	Sniffer_dataC[20][10];//监测数据
									  //基本订单数据
extern	double	Trade_dataB[20][10];//参数


extern double	Trade_Stopwin[20];
extern double	Trade_Stoploss[20];

extern double	Trade_Largevol[20];
extern double	Trade_Littelvol[20];
extern double	Trade_CloseProfit[20];
extern double	Trade_Closetimes[20];
extern	int		Trade_times[20];	//

extern bool		SnifferSignalB[20];
extern bool		TradingSignalB[20];

extern	double	Day_CloseProfitB[20];
extern	double	Trade_StopCloseProfit[20];
extern double	Day_TradeNumb[20];

extern int		Q_BarTime_1;		//

extern	int		Q_BarTime_1n[20];	//

extern	int		simMode;
extern	int		DebugMode;

extern char		InstrumentID_n[20][10];
extern int		InstrumentID_lots[20];
extern double	InstrumentID_minmove[20];

extern	double  Day_open[20][60];	//日K线开
extern	double  Day_high[20][60];	//日K线高
extern	double  Day_low[20][60];	//日K线低
extern	double  Day_close[20][60];	//日K线收

extern	double  Mn_open[20][60];	//分钟K线开
extern	double  Mn_high[20][60];	//分钟K线高
extern	double  Mn_low[20][60];		//分钟K线低
extern	double  Mn_close[20][60];	//分钟K线收

extern  double  BuyPrice[20];		//开仓价
extern  double  SellPrice[20];		//开仓价
extern	int		BNum[20];			//开仓次数
extern	int		SNum[20];			//开仓次数
extern	int		BNum2[20];			//开仓次数
extern	int		SNum2[20];			//开仓次数

extern double	tick_data[20][10];				//TICK基本数据
extern	double	tick_AskPrice1[20][60];			//买一价
extern	double	tick_BidPrice1[20][60];			//卖一价
extern	double	tick_AskVolume1[20][60];		//买一量
extern	double	tick_BidVolume1[20][60];		//卖一量
extern	double	tick_Volume[20][60];			//成交量
extern	double	tick_OpenInterest[20][60];		//持仓量

extern	double	Sniffer_dataA[20][10];//监测数据
extern	double	Sniffer_dataB[20][10];//监测数据
extern	double	Sniffer_dataC[20][10];//监测数据
									  //基本订单数据
extern	double	Trade_dataA[20][10];//参数
extern	double	Trade_dataB[20][10];//参数

extern double	Trade_Stopwin[20];
extern double	Trade_Stoploss[20];

extern double	Trade_Largevol[20];
extern double	Trade_Littelvol[20];
extern double	Trade_CloseProfit[20];
extern double	Trade_Closetimes[20];
extern	int		Trade_times[20];	//

extern bool		SnifferSignalA[20];
extern bool		TradingSignalA[20];

extern	double	Day_CloseProfit[20];
extern	double	Trade_StopCloseProfit[20];


extern	double	Day_BuyPos[20];
extern	double	Day_SellPos[20];
