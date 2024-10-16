﻿#include "Simulation.h"

#define EPSILON_E4 (float)(1E-2) 
#define EPSILON_E5 (float)(1E-3)
bool testfile = false;
//运行模式选择，0=本地测试，1=实盘运行 ，提示Common.h：设置InstrumentID_En=0，实盘在线仿真模式，InstrumentID_En=1，实盘在线交易模式，
int simMode = 0;
CThostFtdcTraderApi* usrApi;
CThostFtdcMdApi* mdApi;
int iRequestID = 0; //请求编号
char FRONT_ADDR_1A[] = "tcp://180.168.212.51:41205"; // 前置地址1交易:实盘
char FRONT_ADDR_1B[] = "tcp://180.168.212.51:41213"; // 前置地址1行情:实盘
// 交易时间
TThostFtdcDateExprType TradingDay; //交易日期

bool JustRun = false; //正在启动标志
bool CloseAll = false; //收盘标志

int FirstVolume = 0; //前一次成交量数据

string InstrumentID_name = ""; //缓存TICK合约名称

int Q_BarTime_1 = 0; //缓存TICK时间戳：秒计算
double Q_BarTime_2 = 0; //缓存TICK时间戳：0.145500

int Q_BarTime_1n[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //合约时间戳：秒计算
int Trade_times[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //开仓时间戳：秒计算

char LogFilePaths[80] = ""; //交易日志
char TickFileWritepaths[20][80] = { "", "", "", "", "", "", "", "", "", "" , "", "", "", "", "", "", "", "", "", "" }; //TICK数据保存文件名次格式，合约名称_日期.txt

//                                 0        1         2        3        4          5        6         7        8         9         10      11       12        13       14        15        16        17        18        19
char InstrumentID_n[20][10] = { "i1409", "jm1409", "j1409", "rb1410", "rb1501", "TA409", "l1409", "ru1409", "ru1501", "jd1409", "RM409", "m1409", "y1501", "p1501", "ag1506", "ag1412", "cu1408", "cu1409", "IF1409", "IF1407" }; //交易合约

int InstrumentID_En[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //交易使能 =1，会实盘下单
int InstrumentID_lots[20] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }; //开仓量

double InstrumentID_minmove[20] = { 1, 1, 1, 1, 1, 2, 5, 5, 5, 1, 1, 1, 2, 2, 1, 1, 10, 10, 0.2, 0.2, }; //最小变动价位

double Trade_Stopwin[20] = { 30, 30, 30, 20, 20, 30, 30, 120, 120, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 }; //单次开仓止赢点
double Trade_Stoploss[20] = { 30, 30, 30, 20, 20, 30, 30, 180, 180, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 }; //单次开仓止损点

double Trade_StopCloseProfit[20] = { 30, 30, 30, 10, 10, 30, 30, 110, 110, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 }; //>止损, 限制新开

//tick数据
bool FristTick[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //收到当日第一个有效TICK标记
bool LastTick[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };    //收到当日最后一个有效TICK标记
bool ReceiveTick[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //TICK数据接收标记，暂未使用

double tick_data[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //行情数据:基本信息

double tick_AskPrice1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };    //行情数据:保存60个TICK数据
double tick_BidPrice1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };    //行情数据:保存60个TICK数据
double tick_AskVolume1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //行情数据:保存60个TICK数据
double tick_BidVolume1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //行情数据:保存60个TICK数据
double tick_Volume[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };       //行情数据:保存60个TICK数据
double tick_OpenInterest[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //行情数据:保存60个TICK数据

double Sniffer_dataA[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //监测数据
double Sniffer_dataB[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //监测数据
double Sniffer_dataC[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //监测数据
double Sniffer_dataD[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //监测数据

//----------------------
double Day_open[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //日K线数据开 
double Day_high[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //日K线数据高
double Day_low[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //日K线数据低
double Day_close[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //日K线数据收

bool MnKlinesig[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };     //分钟K线第一个TICK标记
double Mn_open[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //分钟K线数据开 
double Mn_high[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //分钟K线数据高
double Mn_low[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //分钟K线数据低
double Mn_close[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //分钟K线数据收

//-----------------------
bool SnifferSignalA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //指标策略运算标记
bool TradingSignalA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //下单标记
bool SnifferSignalB[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //指标策略运算标记
bool TradingSignalB[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //下单标记

//基本订单数据
double Trade_dataA[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //订单数据
double Trade_dataB[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //订单数据
double Trade_dataC[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //订单数据
double Trade_dataD[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //订单数据


//测试开平仓统计数据
double Trade_CloseProfit[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; //平仓盈亏，测试策略用
double Trade_Closetimes[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //平仓次数，测试策略用
double Day_CloseProfit[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };   //当日平仓收益，测试策略用
double Day_CloseProfitA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //当日平仓收益，测试策略用
double Day_CloseProfitB[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //当日平仓收益，测试策略用
double Day_TradeNumb[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };     //开仓次数统计

static bool g_dingShiEn = false; //调试用
static bool g_winNen = true;     //调试用
static bool g_lossEn = false;    //调试用
static bool g_fanNen = true;     //调试用
static bool g_duoWen = true;     //调试用
static bool g_konGen = true;     //调试用

void sim_test()
{
    void Simulation_onefile(char* pathstt);

    _finddata_t file;
    long lf;

    //输入文件夹路径 
    if ((lf = _findfirst("..\\Bin\\testdata\\*.*", &file)) == -1)
        cout << "Not Found!" << endl;
    else {
        int n = 0;
        while (_findnext(lf, &file) == 0) {
            cout << file.name << endl;
            Sleep(1000);

            if (n > 0) {
                cout << "准备测试第" << n << "个文件！" << endl;

                string str = file.name;
                string str2 = ".txt";

                size_t iPos = str.find(".");
                string str0 = str.substr(iPos, 4);

                if (str0 == str2.c_str()) {
                    testfile = true;
                } else {
                    testfile = false;
                }
                char* str1 = file.name;
                Simulation_onefile(str1);
            }
            n = n + 1;
        }
    }
    _findclose(lf);
}

void _record0(char* txt)
{
    ofstream o_file(LogFilePaths, ios::app);
    o_file << txt << endl;
    o_file.close();
}

void Sniffer_A12(int i) //监听Tick数据以及指标计算
{
    void Sniffer_56(int i); //

    if (!ReceiveTick[i] && fabs(tick_data[i][0] - 1) < 0.01) {
        SnifferSignalA[i] = true;

        bool TradingTimeA = (tick_data[i][2] > 0.0910 && tick_data[i][2] < 0.1450) || (tick_data[i][2] > 0.2105 && tick_data[i][2] < 0.2359);

        if (simMode && Mn_close[i][1] > Mn_open[i][1] && TradingTimeA) {
            Sniffer_dataA[i][0] = 1;
        } else if (simMode && Mn_close[i][1] < Mn_open[i][1] && TradingTimeA) {
            Sniffer_dataA[i][0] = 2;
        } else {
            Sniffer_dataA[i][0] = 0;
            SnifferSignalA[i] = false;
        }
    }
}

void Sniffer_B12(int i) //监听Tick数据以及指标计算
{
    if (!ReceiveTick[i] && fabs(tick_data[i][0] - 1) < 0.01)//ru
    {
        SnifferSignalB[i] = true;

        bool TradingTimeA = (tick_data[i][2] > 0.0910 && tick_data[i][2] < 0.1450) || (tick_data[i][2] > 0.2105 && tick_data[i][2] < 0.2359);
        bool TradingTimeB = (tick_data[i][2] > 0.0910 && tick_data[i][2] < 0.1450) || (tick_data[i][2] > 0.2105 && tick_data[i][2] < 0.2359);

        bool condtion1 = tick_AskPrice1[i][0] > tick_AskPrice1[i][1];
        bool condtion2 = tick_AskPrice1[i][0] < tick_AskPrice1[i][1];

        bool condtion3 = tick_AskPrice1[i][1] > tick_AskPrice1[i][2];
        bool condtion4 = tick_AskPrice1[i][1] < tick_AskPrice1[i][2];

        bool condtion5 = tick_AskPrice1[i][2] > tick_AskPrice1[i][3];
        bool condtion6 = tick_AskPrice1[i][2] < tick_AskPrice1[i][3];

        cout << "condtion3~6=[" << condtion3 << condtion4 << condtion5 << condtion6 << "], TradingTimeB=" << TradingTimeB << endl;

        if ((condtion1 && 1) && TradingTimeA) {
            Sniffer_dataB[i][0] = 5;
        } else if ((condtion2 && 1) && TradingTimeA) {
            Sniffer_dataB[i][0] = 6;
        } else {
            Sniffer_dataB[i][0] = 0;
            SnifferSignalB[i] = false;
        }
    }
}

void Sniffer() //监听Tick数据已经指标计算 实盘用
{
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);

    void WriteMdConfiguration();
    void ErasingTradeConfiguration();

    double Nowtime = (double)((sys_time.wHour) / 10e1) + (double)((sys_time.wMinute) / 10e3) + (double)((sys_time.wSecond) / 10e5); //格式时间0.145100

    if (simMode && (tick_data[18][2] >= 0.151459 || tick_data[3][2] >= 0.145959) && Nowtime > 0.1518 && Nowtime < 0.1520 && CloseAll == false) {
        cerr << "--->>> " << ff2ss(tick_data[1][1]) << "准备收盘!" << endl;
        cerr << "--->>> " << "WriteMdConfiguration!" << endl;
        WriteMdConfiguration(); //备份数据
        Sleep(3000);
        ErasingTradeConfiguration();
        cerr << "--->>> " << ff2ss(tick_data[1][1]) << "收盘!" << endl;
        CloseAll = true;
    }
    for (int i = 0; i < 20; i++) {
        if (simMode) {
            if ((fabs(tick_data[i][0] - 1) < 0.01) && ((i > 17 && (tick_data[i][2] > 0.0913)) || (i <= 17 && tick_data[i][2] > 0.0858))) //i>17合约为IF
            {
                Sniffer_A12(i);
                Sniffer_B12(i);
                tick_data[i][0] = 0;
            }
        } else {
            if (fabs(tick_data[i][0] - 1) < 0.01) {
                Sniffer_A12(i);
                Sniffer_B12(i);
                tick_data[i][0] = 0;
            }
        }
    }
}

void TraderA12(double system_times, int i)
{
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int a, int b, int md, int i);
    void WriteTradeConfiguration();
    double GetLocalTimeSec1();
    void _record1(char* txt1, char* txt2, double m, int n, int i);

    bool TradingTimeB = (tick_data[i][2] > 0.1014 && tick_data[i][2] < 0.1400); // || (tick_data[i][2]>0.2105 && tick_data[i][2]<0.2350) ;
    bool TradingTimeS = (tick_data[i][2] > 0.0905 && tick_data[i][2] < 0.1400); // || (tick_data[i][2]>0.2105 && tick_data[i][2]<0.2350) ;

    bool Condtion1 = (tick_BidPrice1[i][0] - tick_data[i][9]) > 20 * InstrumentID_minmove[i] && (tick_data[i][8] - tick_AskPrice1[i][0]) > 10 * InstrumentID_minmove[i];
    bool Condtion2 = (tick_data[i][8] - tick_AskPrice1[i][0]) > 20 * InstrumentID_minmove[i] && (tick_BidPrice1[i][0] - tick_data[i][9]) > 10 * InstrumentID_minmove[i];

    if (Sniffer_dataA[i][0] > 0 && TradingSignalA[i] == false) {
        SnifferSignalA[i] = false;
        TradingSignalA[i] = true;

        if ((Sniffer_dataA[i][0] == 1 || Sniffer_dataA[i][0] == 98) && TradingTimeB) {
            if (Trade_dataA[i][2] < -0.5)//如有反向单，先平仓处理
            {
                SendOrder(InstrumentID_n[i], 0, 3, 1, i);
                TradingSignalA[i] = false;

                Trade_dataA[i][0] = 0;
                Trade_dataA[i][2] = 0;
                Trade_dataA[i][3] = 0;
                Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
                Day_CloseProfit[i] = Day_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
                Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
                Trade_dataA[i][5] = tick_AskPrice1[i][0];
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
                if (simMode) { WriteTradeConfiguration(); }
                _record1("报单:", "买平", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
            }

            if ((fabs(Trade_dataA[i][2])) < 0.5 && Day_CloseProfit[i] > -1.6 * Trade_StopCloseProfit[i] && Condtion1) {
                SendOrder(InstrumentID_n[i], 0, 0, 1, i);
                TradingSignalA[i] = false;

                Trade_dataA[i][0] = 0;
                Trade_dataA[i][1] = tick_data[i][2];
                Trade_times[i] = Q_BarTime_1n[i];
                Trade_dataA[i][2] = 1;
                Trade_dataA[i][3] = 1;
                Trade_dataA[i][5] = tick_AskPrice1[i][0];
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买开" << "_" << Trade_dataA[i][5] << "_" << Trade_dataA[i][3] << endl;

                if (simMode) { WriteTradeConfiguration(); }
                _record1("报单:", "买开", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);

            }
        } else if ((Sniffer_dataA[i][0] == 2 || Sniffer_dataA[i][0] == 98) && TradingTimeS) {
            if (Trade_dataA[i][2] > 0.5)//如有反向单，先平仓处理
            {
                SendOrder(InstrumentID_n[i], 1, 3, 1, i);
                TradingSignalA[i] = false;

                Trade_dataA[i][0] = 0;
                Trade_dataA[i][2] = 0;
                Trade_dataA[i][3] = 0;
                Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
                Day_CloseProfit[i] = Day_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
                Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
                Trade_dataA[i][5] = tick_BidPrice1[i][0];
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
                if (simMode) { WriteTradeConfiguration(); }
                _record1("报单:", "卖平", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
            }

            if ((fabs(Trade_dataA[i][2])) < 0.5 && Day_CloseProfit[i] > -1.6 * Trade_StopCloseProfit[i] && Condtion2) {
                SendOrder(InstrumentID_n[i], 1, 0, 1, i);
                TradingSignalA[i] = false;

                Trade_dataA[i][0] = 0;
                Trade_dataA[i][1] = tick_data[i][2];
                Trade_times[i] = Q_BarTime_1n[i];
                Trade_dataA[i][2] = -1;
                Trade_dataA[i][3] = 2;
                Trade_dataA[i][5] = tick_BidPrice1[i][0];
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖开" << "_" << Trade_dataA[i][5] << "_" << Trade_dataA[i][3] << endl;

                if (simMode) { WriteTradeConfiguration(); }
                _record1("报单:", "卖开", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
            }
        } else {
            TradingSignalA[i] = false;
        }
    }
}

void StopLossA12(double system_times, int i)
{
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int a, int b, int md, int i);
    void WriteTradeConfiguration();
    int GetLocalTimeSec2();
    void _record1(char* txt1, char* txt2, double m, int n, int i);

    double Mn_D2 = (Mn_open[i][1] + Mn_close[i][1]) / 2;

    //止赢平仓
    if (Trade_dataA[i][2] == 1 && Trade_dataA[i][3] == 1 && (Mn_D2 - Trade_dataA[i][5]) >= Trade_Stopwin[i]) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalA[i] = false;

        //Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_dataA[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平止赢" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "卖平止赢", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
    }
    if (Trade_dataA[i][2] == -1 && Trade_dataA[i][3] == 2 && (Trade_dataA[i][5] - Mn_D2) >= Trade_Stopwin[i]) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalA[i] = false;

        //Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_dataA[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平止赢" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "买平止赢", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
    }

    //止损平仓
    if (Trade_dataA[i][2] == 1 && Trade_dataA[i][3] == 1 && (Trade_dataA[i][5] - Mn_D2) >= Trade_Stoploss[i]) {
        SendOrder(InstrumentID_n[i], 1, 3, 1, i);
        TradingSignalA[i] = false;

        //Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_dataA[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平止损" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;

        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "卖平止损", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);

    }
    if (Trade_dataA[i][2] == -1 && Trade_dataA[i][3] == 2 && (Mn_D2 - Trade_dataA[i][5]) >= Trade_Stoploss[i]) {
        SendOrder(InstrumentID_n[i], 0, 3, 1, i);
        TradingSignalA[i] = false;

        //Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_dataA[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平止损" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;

        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "买平止损", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);

    }
}

void StopEndTime_A(double system_times, int i)
{
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int a, int b, int md, int i);
    void WriteTradeConfiguration();
    int GetLocalTimeSec2();
    void _record1(char* txt1, char* txt2, double m, int n, int i);

    int n = 10; //10分钟定时平仓

    //定时平仓
    if (Trade_dataA[i][2] == 1 && (Q_BarTime_1n[i] - Trade_times[i]) > n * 60 && g_dingShiEn) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalA[i] = false;

        Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_dataA[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平定时" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "卖平定时", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
    }
    if (Trade_dataA[i][2] == -1 && (Q_BarTime_1n[i] - Trade_times[i]) > n * 60 && g_dingShiEn) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalA[i] = false;

        Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_dataA[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平定时" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "买平定时", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
    }

    //收盘平仓
    if (Trade_dataA[i][2] == 1 && ((system_times > 0.1455 && system_times < 0.1456) || (system_times > 0.2350 && system_times <= 0.2356))) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalA[i] = false;

        Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataA[i][5];
        Trade_dataA[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平收盘" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        _record1("报单:", "卖平收盘", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
    }
    if (Trade_dataA[i][2] == -1 && ((system_times > 0.1455 && system_times <= 0.1456) || (system_times > 0.2350 && system_times <= 0.2356))) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalA[i] = false;

        Trade_dataA[i][0] = 0;
        Trade_dataA[i][2] = 0;
        Trade_dataA[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfit[i] = Day_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataA[i][5] - tick_AskPrice1[i][0];
        Trade_dataA[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平收盘" << "_" << Trade_dataA[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        _record1("报单:", "买平收盘", Trade_dataA[i][5], int(Trade_dataA[i][3]), i);
    }

    if ((system_times > 0.1455 && system_times < 0.1459) || (system_times > 0.2350 && system_times <= 0.2359)) {
        SnifferSignalA[i] = false;
    }
}

void TraderB12(double system_times, int i)
{
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int a, int b, int md, int i);
    void WriteTradeConfiguration();
    double GetLocalTimeSec1();
    void _record1(char* txt1, char* txt2, double m, int n, int i);

    bool TradingTimeB = (tick_data[i][2] > 0.0920 && tick_data[i][2] < 0.1450) || (tick_data[i][2] > 0.2105 && tick_data[i][2] < 0.2350);
    bool TradingTimeS = (tick_data[i][2] > 0.0920 && tick_data[i][2] < 0.1450) || (tick_data[i][2] > 0.2105 && tick_data[i][2] < 0.2350);

    if (Sniffer_dataB[i][0] > 0 && TradingSignalB[i] == false) {

        SnifferSignalB[i] = false;
        TradingSignalB[i] = true;

        //需加入反手操作
        if ((Sniffer_dataB[i][0] == 5 || Sniffer_dataB[i][0] == 98) && Trade_dataB[i][2] == -1 && Trade_dataB[i][3] == 6) {
            SendOrder(InstrumentID_n[i], 0, 3, 0, i);
            TradingSignalB[i] = false;

            Trade_dataB[i][0] = 0;
            Trade_dataB[i][2] = 0;
            Trade_dataB[i][3] = 0;
            Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
            Day_CloseProfitB[i] = Day_CloseProfitB[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
            Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
            Trade_dataB[i][5] = tick_AskPrice1[i][0];
            cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
            if (simMode) { WriteTradeConfiguration(); }
            _record1("报单:", "买平", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);

            if (TradingTimeB && g_fanNen && g_duoWen && Day_CloseProfitB[i] > -1 * Trade_StopCloseProfit[i]) {
                SendOrder(InstrumentID_n[i], 0, 0, 0, i);
                TradingSignalB[i] = false;

                Trade_dataB[i][0] = 0;
                Trade_dataB[i][1] = tick_data[i][2];
                Trade_times[i] = Q_BarTime_1n[i];
                Trade_dataB[i][2] = 1;
                Trade_dataB[i][3] = 5;
                Trade_dataB[i][5] = tick_AskPrice1[i][0];
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买开" << "_" << Trade_dataB[i][5] << "_" << Trade_dataB[i][3] << endl;
                if (simMode) { WriteTradeConfiguration(); }
                _record1("报单:", "买开", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
            }

        } else if ((Sniffer_dataB[i][0] == 6 || Sniffer_dataB[i][0] == 98) && Trade_dataB[i][2] == 1 && Trade_dataB[i][3] == 5) {
            SendOrder(InstrumentID_n[i], 1, 3, 0, i);
            TradingSignalB[i] = false;

            Trade_dataB[i][0] = 0;
            Trade_dataB[i][2] = 0;
            Trade_dataB[i][3] = 0;
            Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
            Day_CloseProfitB[i] = Day_CloseProfitB[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
            Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
            Trade_dataB[i][5] = tick_BidPrice1[i][0];
            cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
            if (simMode) { WriteTradeConfiguration(); }
            _record1("报单:", "卖平", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);

            if (TradingTimeS && g_fanNen && g_konGen && Day_CloseProfitB[i] > -1 * Trade_StopCloseProfit[i]) {
                SendOrder(InstrumentID_n[i], 1, 0, 0, i);
                TradingSignalB[i] = false;

                Trade_dataB[i][0] = 0;
                Trade_dataB[i][1] = tick_data[i][2];
                Trade_times[i] = Q_BarTime_1n[i];
                Trade_dataB[i][2] = -1;
                Trade_dataB[i][3] = 6;
                Trade_dataB[i][5] = tick_BidPrice1[i][0];
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖开" << "_" << Trade_dataB[i][5] << "_" << Trade_dataB[i][3] << endl;
                if (simMode) { WriteTradeConfiguration(); }
                _record1("报单:", "卖开", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
            }

        } else if ((Sniffer_dataB[i][0] == 5 || Sniffer_dataB[i][0] == 98) && Trade_dataB[i][2] == 0 && TradingTimeB && g_duoWen && Day_CloseProfitB[i] > -1 * Trade_StopCloseProfit[i]) {

            SendOrder(InstrumentID_n[i], 0, 0, 0, i);
            TradingSignalB[i] = false;

            Trade_dataB[i][0] = 0;
            Trade_dataB[i][1] = tick_data[i][2];
            Trade_times[i] = Q_BarTime_1n[i];
            Trade_dataB[i][2] = 1;
            Trade_dataB[i][3] = 5;
            Trade_dataB[i][5] = tick_AskPrice1[i][0];
            cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买开" << "_" << Trade_dataB[i][5] << "_" << Trade_dataB[i][3] << endl;
            if (simMode) { WriteTradeConfiguration(); }
            _record1("报单:", "买开", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);

        } else if ((Sniffer_dataB[i][0] == 6 || Sniffer_dataB[i][0] == 98) && Trade_dataB[i][2] == 0 && TradingTimeS && g_konGen && Day_CloseProfitB[i] > -1 * Trade_StopCloseProfit[i]) {

            SendOrder(InstrumentID_n[i], 1, 0, 0, i);
            TradingSignalB[i] = false;

            Trade_dataB[i][0] = 0;
            Trade_dataB[i][1] = tick_data[i][2];
            Trade_times[i] = Q_BarTime_1n[i];
            Trade_dataB[i][2] = -1;
            Trade_dataB[i][3] = 6;
            Trade_dataB[i][5] = tick_BidPrice1[i][0];
            cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖开" << "_" << Trade_dataB[i][5] << "_" << Trade_dataB[i][3] << endl;
            if (simMode) { WriteTradeConfiguration(); }
            _record1("报单:", "卖开", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);

        } else {
            TradingSignalB[i] = false;
        }
    }
}

void StopLossB12(double system_times, int i)
{
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int a, int b, int md, int i);
    void WriteTradeConfiguration();
    int GetLocalTimeSec2();
    void _record1(char* txt1, char* txt2, double m, int n, int i);

    //止赢平仓
    if (Trade_dataB[i][2] == 1 && Trade_dataB[i][3] == 5 && (tick_BidPrice1[i][0] - Trade_dataB[i][5]) >= Trade_Stopwin[i] && g_winNen) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalB[i] = false;

        //Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_dataB[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平止赢" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "卖平止赢", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
    }
    if (Trade_dataB[i][2] == -1 && Trade_dataB[i][3] == 6 && (Trade_dataB[i][5] - tick_AskPrice1[i][0]) >= Trade_Stopwin[i] && g_winNen) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalB[i] = false;

        //Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_dataB[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平止赢" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "买平止赢", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
    }

    //止损平仓
    if (Trade_dataB[i][2] == 1 && Trade_dataB[i][3] == 5 && (Trade_dataB[i][5] - tick_BidPrice1[i][0]) >= Trade_Stoploss[i] && g_lossEn) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalB[i] = false;

        //Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_dataB[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平止损" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "卖平止损", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);

    }
    if (Trade_dataB[i][2] == -1 && Trade_dataB[i][3] == 6 && (tick_AskPrice1[i][0] - Trade_dataB[i][5]) >= Trade_Stoploss[i] && g_lossEn) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalB[i] = false;

        //Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_dataB[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平止损" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "买平止损", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);

    }
}

void StopEndTime_B(double system_times, int i)
{
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int a, int b, int md, int i);
    void WriteTradeConfiguration();
    int GetLocalTimeSec2();
    void _record1(char* txt1, char* txt2, double m, int n, int i);

    int n = 10; //10分钟定时平仓

    //定时平仓
    if (Trade_dataB[i][2] == 1 && (Q_BarTime_1n[i] - Trade_times[i]) > n * 60 && g_dingShiEn) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalB[i] = false;

        Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_dataB[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平定时" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "卖平定时", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
    }
    if (Trade_dataB[i][2] == -1 && (Q_BarTime_1n[i] - Trade_times[i]) > n * 60 && g_dingShiEn) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalB[i] = false;

        Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_dataB[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平定时" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        if (simMode) { WriteTradeConfiguration(); }
        _record1("报单:", "买平定时", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
    }

    //收盘平仓
    if (Trade_dataB[i][2] == 1 && ((system_times > 0.1455 && system_times < 0.1456) || (system_times > 0.2350 && system_times <= 0.2356))) {
        SendOrder(InstrumentID_n[i], 1, 3, 0, i);
        TradingSignalB[i] = false;

        Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + tick_BidPrice1[i][0] - Trade_dataB[i][5];
        Trade_dataB[i][5] = tick_BidPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "卖平收盘" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        _record1("报单:", "卖平收盘", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
    }
    if (Trade_dataB[i][2] == -1 && ((system_times > 0.1455 && system_times <= 0.1456) || (system_times > 0.2350 && system_times <= 0.2356))) {
        SendOrder(InstrumentID_n[i], 0, 3, 0, i);
        TradingSignalB[i] = false;

        Trade_dataB[i][0] = 0;
        Trade_dataB[i][2] = 0;
        Trade_dataB[i][3] = 0;
        Trade_Closetimes[i] = Trade_Closetimes[i] + 1;
        Day_CloseProfitB[i] = Day_CloseProfitB[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_CloseProfit[i] = Trade_CloseProfit[i] + Trade_dataB[i][5] - tick_AskPrice1[i][0];
        Trade_dataB[i][5] = tick_AskPrice1[i][0];
        cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << tick_data[i][2] << "_" << "买平收盘" << "_" << Trade_dataB[i][5] << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
        _record1("报单:", "买平收盘", Trade_dataB[i][5], int(Trade_dataB[i][3]), i);
    }

    if ((system_times > 0.1455 && system_times < 0.1459) || (system_times > 0.2350 && system_times <= 0.2359)) {
        SnifferSignalB[i] = false;
        Day_CloseProfitB[i] = 0;
    }
}

void Istrading() //策略，下单
{
    for (int i = 0; i < 20; i++) {
        if (1) //i!=8 && i!=9) //测试用，暂时不交易ag，cu, 对仿真，实盘均有效
        {

            StopEndTime_A(tick_data[i][2], i);
            StopEndTime_B(tick_data[i][2], i);

            if (simMode) {
                if ((i > 17 && (tick_data[i][2] > 0.0913)) || (i <= 17 && tick_data[i][2] > 0.0858)) {
                    StopLossA12(tick_data[i][2], i); //Master
                    if (Sniffer_dataA[i][0] > 0) {
                        TraderA12(tick_data[i][2], i); //Master
                        Sniffer_dataA[i][0] = 0;
                    }

                    StopLossB12(tick_data[i][2], i);
                    if (Sniffer_dataB[i][0] > 0) {
                        TraderB12(tick_data[i][2], i);
                        Sniffer_dataB[i][0] = 0;
                    }
                }
            } else //测试模式
            {
                StopLossA12(tick_data[i][2], i); //Master
                if (Sniffer_dataA[i][0] > 0) {
                    TraderA12(tick_data[i][2], i); //Master
                    Sniffer_dataA[i][0] = 0;
                }

                StopLossB12(tick_data[i][2], i);
                if (Sniffer_dataB[i][0] > 0) {
                    TraderB12(tick_data[i][2], i);
                    Sniffer_dataB[i][0] = 0;
                }
            }
        }
    }
}

void Simulation_onefile(char* pathstt)
{
    void _record0(char* txt);

    char Readfilepaths[50] = "";
    strcpy(Readfilepaths, "./testdata/");
    strcat(Readfilepaths, pathstt);

    ifstream fin(Readfilepaths, std::ios::in);

    string str = pathstt;
    size_t iPos = str.find("_");
    string str0 = str.substr(0, iPos);

    strcpy(LogFilePaths, "./Simulation/Simulation_");
    strcat(LogFilePaths, str0.c_str());
    strcat(LogFilePaths, ".txt");

    Sleep(3000);
    char line[1024] = { 0 };
    vector < double > data(16);

    _record0("开始测试！！！");
    int t0 = 0;

    while (fin.getline(line, sizeof(line))) {
        std::stringstream word(line);
        t0 = t0 + 1;

        for (int i = 0; i < 20; i++) {

            if (InstrumentID_n[i] == str0 && testfile) {

                InstrumentID_name = InstrumentID_n[i];
                tick_data[i][0] = 1; //设置标志位
                ReceiveTick[i] = true;

                for (int j = 0; j < 10; j++) {
                    word >> data[j];
                    //cout << "Configuration:" << data[j] << endl;
                }

                Q_BarTime_1n[i] = (int(data[1] * 100)) * 60 * 60 + (int((data[1] * 100 - int(data[1] * 100)) * 100)) * 60 + (int((data[1] * 10000 - int(data[1] * 10000)) * 100));

                tick_data[i][1] = data[0];
                tick_data[i][2] = data[1];

                if ((tick_data[i][2] > 0.0856 && tick_data[i][2] < 0.0900 && i <= 13) || (tick_data[i][2] > 0.2056 && tick_data[i][2] < 0.2100 && i>13 && i <= 17) || (tick_data[i][2] > 0.0913 && tick_data[i][2] < 0.0915 && i>17)) {
                    Day_open[i][0] = data[3];
                    Day_CloseProfit[i] = 0;
                    Day_CloseProfitA[i] = 0;
                    Day_CloseProfitB[i] = 0;

                }

                for (int j = 59; j > 0; j--) {
                    tick_AskPrice1[i][j] = tick_AskPrice1[i][j - 1];
                    tick_BidPrice1[i][j] = tick_BidPrice1[i][j - 1];
                    tick_AskVolume1[i][j] = tick_AskVolume1[i][j - 1];
                    tick_BidVolume1[i][j] = tick_BidVolume1[i][j - 1];
                    tick_Volume[i][j] = tick_Volume[i][j - 1];
                    tick_OpenInterest[i][j] = tick_OpenInterest[i][j - 1];
                }

                tick_AskPrice1[i][0] = data[3];
                tick_BidPrice1[i][0] = data[5];
                tick_AskVolume1[i][0] = data[4];
                tick_BidVolume1[i][0] = data[6];
                tick_Volume[i][0] = data[8];
                tick_OpenInterest[i][0] = data[9];

                //cout << "Configuration:" << tick_data[i][2] << endl;
                ReceiveTick[i] = false;
                //}

                Sniffer();
                Istrading();
                Sniffer();
                Istrading(); //重复是为了和实际一致，1个tick收到后策略可能被多次运行。
                //Sleep(100);
                //cout << "Configuration:2_" << setprecision(9) <<GetLocalTimeMs1() << endl; 
                //打印某个数据 
                //鎵撳嵃鏌愪釜鏁版嵁
            }
        }
    }
    fin.clear();
    fin.close();
}

void _record1(char* txt1, char* txt2, double m, int n, int i)
{
    string ff2ss(double nums);

    ofstream o_file(LogFilePaths, ios::app);
    if (n > 0) {
        //将内容写入到文本文件中
        if (n <= 4) {
            o_file << txt1 << ff2ss(tick_data[i][1]) << "_" << tick_data[i][2] << "_" << InstrumentID_n[i] << "_" << InstrumentID_lots[i] << "_" << txt2 << "_" << Trade_dataA[i][5] << "_" << Trade_dataA[i][3] << endl;
        } else if (n <= 6) {
            o_file << txt1 << ff2ss(tick_data[i][1]) << "_" << tick_data[i][2] << "_" << InstrumentID_n[i] << "_" << InstrumentID_lots[i] << "_" << txt2 << "_" << Trade_dataB[i][5] << "_" << Trade_dataB[i][3] << endl;
        }
    } else {
        // --待修改！！！
        o_file << txt1 << ff2ss(tick_data[i][1]) << "_" << tick_data[i][2] << "_" << InstrumentID_n[i] << "_" << InstrumentID_lots[i] << "_" << txt2 << "_" << m << "_" << Trade_Closetimes[i] << "_" << Trade_CloseProfit[i] << endl;
    }
    o_file.close();
}

double GetLocalTimeSec1()
{
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    double system_times;
    system_times = (double)((sys_time.wHour) / 10e1) + (double)((sys_time.wMinute) / 10e3) + (double)((sys_time.wSecond) / 10e5); //格式化时间0.145100
    return system_times;
}

int GetLocalTimeSec2()
{
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    int system_times;
    system_times = (int)((sys_time.wHour) * 60 * 60) + (int)((sys_time.wMinute) * 60) + (int)((sys_time.wSecond)); //格式时间int sec
    return system_times;
}

double GetLocalTimeMs1()
{
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    double system_times;
    system_times = (double)((sys_time.wHour) / 10e1) + (double)((sys_time.wMinute) / 10e3) + (double)((sys_time.wSecond) / 10e5) + (double)((sys_time.wMilliseconds) / 10e8);
    return system_times;
}

int GetLocalTimeMs2()
{
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    int system_times;
    system_times = ((int)((sys_time.wHour) * 60 * 60) + (int)((sys_time.wMinute) * 60) + (int)((sys_time.wSecond))) * 1000 + (int)((sys_time.wMilliseconds)); //格式时间int ms
    return system_times;
}

bool ReadMdConfiguration()
{
    ifstream config("./AutoTrader.dat");

    if (!config) {
        cerr << "--->>> " << "MdConfiguration File is missing!" << endl;
        return false;
    } else {
        cerr << "--->>> " << "Read MdConfiguration File!" << endl;
    }

    char line[1024] = { 0 };

    ifstream fin("./AutoTrader.dat", std::ios::in);
    int i = 0;

    while (fin.getline(line, sizeof(line))) {
        std::stringstream word(line);
        double Day_fprice_t[9];

        for (int j = 0; j < 9; j++) {
            word >> Day_fprice_t[j];
            //cout << Day_fprice_t[i][j] << endl;
        }

        Day_open[i][1] = Day_fprice_t[5];
        Day_high[i][1] = Day_fprice_t[6];
        Day_low[i][1] = Day_fprice_t[7];
        Day_close[i][1] = Day_fprice_t[8];

        Day_open[i][2] = Day_fprice_t[1];
        Day_high[i][2] = Day_fprice_t[2];
        Day_low[i][2] = Day_fprice_t[3];
        Day_close[i][2] = Day_fprice_t[4];

        i++;
    }
    config.close();
    return true;
}

bool ReadTradeConfiguration()
{
    ifstream config("./AutoTrader.cfg");

    if (!config) {
        cerr << "--->>> " << "TradeConfiguration File is missing!" << endl;
        return false;
    } else {
        cerr << "--->>> " << "Read TradeConfiguration File!" << endl;
    }

    char line[1024] = { 0 };
    // vector < double > data(10);
    double temp0 = 0;
    double temp1 = 0;

    ifstream fin("./AutoTrader.cfg", std::ios::in);
    int it = 0;

    while (fin.getline(line, sizeof(line))) {
        int i = int(it % 20);
        std::stringstream word(line);

        for (int j = 0; j < 9; j++) {
            if (j == 0) {
                word >> temp0;
            } else if (j == 1) {
                word >> temp1;
            } else if (j > 1 && j <= 6) {
                if (it < 20) {
                    word >> Trade_dataA[i][j - 1];
                } else if (it < 40) {
                    word >> Trade_dataB[i][j - 1];
                } else if (it < 60) {
                    word >> Trade_dataC[i][j - 1];
                }

            } else if (j == 7) {
                word >> Trade_Closetimes[i];
            } else if (j == 8) {
                word >> Trade_CloseProfit[i];
            }
        }

        if (temp1 > 0 && temp1 < 5) {
            if (Trade_dataA[i][2] > 0.5) {
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << Trade_dataA[i][1] << "_" << Trade_dataA[i][3] << "_" << "买开" << "_" << Trade_dataA[i][5] << endl;
            } else if (Trade_dataA[i][2] < -0.5) {
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << Trade_dataA[i][1] << "_" << Trade_dataA[i][3] << "_" << "卖开" << "_" << Trade_dataA[i][5] << endl;
            }
        } else if (temp1 > 4 && temp1 < 7) {
            if (Trade_dataB[i][2] > 0.5) {
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << Trade_dataB[i][1] << "_" << Trade_dataB[i][3] << "_" << "买开" << "_" << Trade_dataB[i][5] << endl;
            } else if (Trade_dataB[i][2] < -0.5) {
                cerr << "--->>> 报单: " << InstrumentID_n[i] << "_" << Trade_dataB[i][1] << "_" << Trade_dataB[i][3] << "_" << "卖开" << "_" << Trade_dataB[i][5] << endl;
            }
        }
        it++;
    }
    config.close();
    return true;
}

void WriteMdConfiguration()
{
    string ff2ss(double nums);

    ofstream o_file("./AutoTrader.dat", ios::trunc); //保存近2天的日线数据
    for (int i = 0; i < 20; i++) {
        Day_open[i][2] = Day_open[i][1];
        Day_high[i][2] = Day_high[i][1];
        Day_low[i][2] = Day_low[i][1];
        Day_close[i][2] = Day_close[i][1];

        Day_open[i][1] = Day_open[i][0];
        Day_high[i][1] = Day_high[i][0];
        Day_low[i][1] = Day_low[i][0];
        Day_close[i][1] = Day_close[i][0];

        //o_file << ff2ss(tick_data[0][1]) << "\t" <<tick_data[i][2] <<"\t"<< Mn_open << " " << Mn_high << " " << Mn_low << " " << Mn_close << endl;
        o_file << ff2ss(tick_data[i][1]) << "\t" << Day_open[i][2] << "\t" << Day_high[i][2] << "\t" << Day_low[i][2] << "\t" << Day_close[i][2] << "\t" << Day_open[i][1] << "\t" << Day_high[i][1] << "\t" << Day_low[i][1] << "\t" << Day_close[i][1] << endl;
    }
    o_file.close();
}

void WriteTradeConfiguration()
{
    string ff2ss(double nums);

    ofstream o_file("./AutoTrader.cfg", ios::trunc);
    for (int i = 0; i < 20; i++) {
        o_file << ff2ss(tick_data[i][1]) << "\t" << Trade_dataA[i][3] << "\t" << Trade_dataA[i][1] << "\t" << Trade_dataA[i][2] << "\t" << Trade_dataA[i][3] << "\t" << Trade_dataA[i][4] << "\t" << Trade_dataA[i][5] << "\t" << Trade_Closetimes[i] << "\t" << Trade_CloseProfit[i] << endl;
    }

    for (int i = 0; i < 20; i++) {
        o_file << ff2ss(tick_data[i][1]) << "\t" << Trade_dataB[i][3] << "\t" << Trade_dataB[i][1] << "\t" << Trade_dataB[i][2] << "\t" << Trade_dataB[i][3] << "\t" << Trade_dataB[i][4] << "\t" << Trade_dataB[i][5] << "\t" << Trade_Closetimes[i] << "\t" << Trade_CloseProfit[i] << endl;
    }
    o_file.close();
}

void ErasingTradeConfiguration()
{
    int tn = 0;
    ofstream o_file("./AutoTrader.cfg", ios::trunc);

    for (int i = 0; i < 20; i++) {
        o_file << ff2ss(tick_data[i][1]) << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << endl;
    }

    for (int i = 0; i < 20; i++) {
        o_file << ff2ss(tick_data[i][1]) << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << "\t" << tn << endl;
    }
    o_file.close();
}

void Erasefiles()
{
    system("del .\\thostmduserapi.dllDialogRsp.con");
    system("del .\\thostmduserapi.dllQueryRsp.con");
    system("del .\\thostmduserapi.dllTradingDay.con");

    system("del .\\thosttraderapi.dllDialogRsp.con");
    system("del .\\thosttraderapi.dllPrivate.con");
    system("del .\\thosttraderapi.dllPublic.con");
    system("del .\\thosttraderapi.dllQueryRsp.con");
    system("del .\\thosttraderapi.dllTradingDay.con");
}

//报单-限价
void SendOrder(TThostFtdcInstrumentIDType FuturesId, int BuySell, int OpenClose, int md, int i)
{

    strcpy(STFTDC.INSTRUMENT_ID, InstrumentID_n[i]);
    //cerr << "--->>> 报单录入请求: " <<INSTRUMENT_ID<< endl;

    STFTDC.DIRECTION = BuySell;
    STFTDC.MARKETState = OpenClose;

    //Sleep(050);

    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    ///投资者代码
    strcpy_s(req.InvestorID, STFTDC.INVESTOR_ID);
    ///合约代码 //INSTRUMENT_ID
    strcpy_s(req.InstrumentID, STFTDC.INSTRUMENT_ID);
    ///报单引用
    //strcpy_s(req.OrderRef, ORDER_REF); //下单前修改
    ///用户代码
    // TThostFtdcUserIDType UserID;
    ///报单价格条件: 限价
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

    ///买卖方向: //THOST_FTDC_D_Buy, THOST_FTDC_D_Sell
    if (BuySell == 0) {
        req.Direction = THOST_FTDC_D_Buy;
    } else if (BuySell == 1) {
        req.Direction = THOST_FTDC_D_Sell;
    }

    ///组合开平标志: 开仓 //THOST_FTDC_OF_Open, THOST_FTDC_OF_Close, THOST_FTDC_OF_CloseToday
    if (OpenClose == 0) {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    } else if (OpenClose == 1) {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_Close; //其他交易所平今，平昨，上期平昨
    } else if (OpenClose == 3) {
        req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday; //上期平今
    }

    ///组合投机套保标志
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation; //投机

    ///价格
    if (md) {
        if (BuySell == 0) {
            STFTDC.LIMIT_PRICE = tick_data[i][8];
        } else if (BuySell == 1) {
            STFTDC.LIMIT_PRICE = tick_data[i][9];
        }
    } else {
        if (BuySell == 0) {
            STFTDC.LIMIT_PRICE = tick_AskPrice1[i][0];
        } else if (BuySell == 1) {
            STFTDC.LIMIT_PRICE = tick_BidPrice1[i][0];
        }
    }

    req.LimitPrice = STFTDC.LIMIT_PRICE;

    ///数量: 1 / 开平仓数量
    req.VolumeTotalOriginal = InstrumentID_lots[i];
    ///有效期类型: 当日有效
    req.TimeCondition = THOST_FTDC_TC_GFD;
    ///GTD日期
    // TThostFtdcDateType GTDDate;
    ///成交量类型: 任何数量
    req.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    req.MinVolume = 1;
    ///触发条件: 立即
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///止损价
    // TThostFtdcPriceType StopPrice;
    ///强平原因: 非强平
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    req.IsAutoSuspend = 0;
    ///业务单元
    // TThostFtdcBusinessUnitType BusinessUnit;
    ///请求编号
    // TThostFtdcRequestIDType RequestID;
    ///用户强评标志: 否
    req.UserForceClose = 0;

    if (simMode && InstrumentID_En[i] == 1) {
        ///报单引用
        int iNextOrderRef = atoi(STFTDC.ORDER_REF);
        iNextOrderRef++;
        sprintf(STFTDC.ORDER_REF, "%d", iNextOrderRef);
        strcpy_s(req.OrderRef, STFTDC.ORDER_REF);
        //sprintf(ORDERACTION_REF[i], ORDER_REF);

        int iResult = usrApi->ReqOrderInsert(&req, ++iRequestID); //实盘，会正式下单
        cerr << "--->>> 报单录入请求: " << InstrumentID_n[i] << ((iResult == 0) ? " 成功" : " 失败") << endl;
    } else {
        //测试，不真正下单
        cerr << "--->>> 报单录入请求成功: " << InstrumentID_n[i] << endl;
    }
}

void Simulation()
{
    void Erasefiles();
    bool ReadMdConfiguration();
    bool ReadTradeConfiguration();
    void _record0(char* txt);
    void Sniffer();
    void SendOrder(TThostFtdcInstrumentIDType FuturesId, int BuySell, int OpenClose, int i);


    JustRun = true; //正在启动标志
    Erasefiles();
    Sleep(2000);

    cerr << "--->>> " << "Welcom MyAutoTrader System!" << endl;
    cerr << "--->>> " << "Version 1.0.3!" << endl;

    ReadMdConfiguration();
    Sleep(2000);
    // 初始化UserApi
    usrApi = CThostFtdcTraderApi::CreateFtdcTraderApi("./thosttraderapi.dll"); // 创建UserApi//"./thosttraderapi.dll"
    TradeChannel* trdSpi = new TradeChannel();
    usrApi->RegisterSpi((CThostFtdcTraderSpi*)trdSpi); // 注册事件类
    usrApi->SubscribePublicTopic(THOST_TERT_RESTART); // 注册公有流
    usrApi->SubscribePrivateTopic(THOST_TERT_RESTART); // 注册私有流
    usrApi->RegisterFront(FRONT_ADDR_1A); // connect
    //usrApi->RegisterFront(FRONT_ADDR_2A); // connect

    if (simMode) {
        usrApi->Init();
        cerr << "--->>> " << "Initialing UserApi" << endl;
    }

    // 初始化MdApi
    mdApi = CThostFtdcMdApi::CreateFtdcMdApi("./thostmduserapi.dll"); // 创建MdApi//"./thostmduserapi.dll"
    MarketDataCollector* mdSpi = new MarketDataCollector();
    mdApi->RegisterSpi(mdSpi); // 注册事件类
    mdApi->RegisterFront(FRONT_ADDR_1B); // connect
    //mdApi->RegisterFront(FRONT_ADDR_2B); // connect

    if (simMode) {
        mdApi->Init();
        cerr << "--->>> " << "Initialing MdApi" << endl;
    } else {
        cerr << "--->>> " << "Test Mode!" << endl;
    }

    //mdApi->Join();
    //mdApi->Release();

    Sleep(7000);
    ReadTradeConfiguration();
    Sleep(1000);
    cerr << "--->>> " << "初始化完成!" << endl;

    while (simMode) //实盘
    {
        Sniffer();
        Istrading();
        Sleep(050);
    }

    while (!simMode) //本地测试
    {
        if (JustRun == true) {
            sim_test();
            JustRun = false;
        }
    }
}
