#pragma once
#include	<iostream>
#include	<fstream>
#include	<sstream>
#include	<cstring>
#include	<ctime>
#include	<Windows.h>
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"

using namespace std;

typedef char TThostFtdcFrontAddrType[64];

struct st_TThostFtdc {
	int iReqID = 0;
	int		MARKETState = 0;
	const char* FLOW_PATH = "./"/*"*.con"*/;
	bool ISMULTICAST = false;
	CThostFtdcTraderApi *TrdApi;
	TThostFtdcFrontAddrType FRONT_ADDR = "tcp://180.168.146.187:10000";	// 前置地址
	TThostFtdcBrokerIDType	BROKER_ID = "88888888";						// 经纪公司代码
	TThostFtdcInvestorIDType INVESTOR_ID = "092380";					// 注意输入你自己的simnow仿真投资者（用户）代码
	TThostFtdcUserIDType USER_ID = "tsymiar";
	TThostFtdcPasswordType  PASSWORD = "Minctp905";						// 注意输入你自己的simnow仿真用户密码
	TThostFtdcInstrumentIDType INSTRUMENT_ID = "i1409";					// 合约代码 ，注意与时俱进改变合约ID,避免使用过时合约
	TThostFtdcDirectionType	DIRECTION = THOST_FTDC_D_Sell;				// 买卖方向
	TThostFtdcPriceType	LIMIT_PRICE = 2380;								// 价格
	TThostFtdcFrontIDType	FRONT_ID;	//前置编号
	TThostFtdcSessionIDType	SESSION_ID;	//会话编号
	TThostFtdcOrderRefType	ORDER_REF;	//报单引用
	TThostFtdcAuthCodeType AUTHCODE;
#ifdef THOST_FTDCMDAPI_H
	CThostFtdcMdApi *MdApi;
	struct st_MdData {
		//行情订阅列表
		char* Instruments[32] = { "i1409", "jm1409", "j1409", "rb1410","rb1501", "TA409", "l1409","ru1409", "ru1501", "jd1409", "RM409", "m1409","y1501", "p1501","ag1506","ag1412","cu1408","cu1409","IF1409","IF1407" };
		int		MdMode;
		bool	ReceiveTick[20];
		bool	FristTick[20];
		bool	LastTick[20];
		TThostFtdcDateExprType	TradingDay;

		string	InstrumentID_name;	//

		int		Q_BarTime_1;		//
		double	Q_BarTime_2;		//
		int		Q_BarTime_1n[20];	//

		double  Mn_open[20][60];	//分钟K线开
		double  Mn_high[20][60];	//分钟K线高
		double  Mn_low[20][60];		//分钟K线低
		double  Mn_close[20][60];	//分钟K线收
		bool	MnKlinesig[20];		//1分钟K线标志

		double  Day_open[20][60];	//日K线开
		double  Day_high[20][60];	//日K线高
		double  Day_low[20][60];	//日K线低
		double  Day_close[20][60];	//日K线收

		char	LogFilePaths[80];				//交易日志
		char	TickFileWritepaths[20][80];		//
		char	InstrumentID_n[20][10];			//
		double  InstrumentID_minmove[20];		//
		double  InstrumentID_diff[20];			//

		double	tick_data[20][10];				//TICK基本数据
		double	tick_AskPrice1[20][60];			//买一价
		double	tick_BidPrice1[20][60];			//卖一价
		double	tick_AskVolume1[20][60];		//买一量
		double	tick_BidVolume1[20][60];		//卖一量
		double	tick_Volume[20][60];			//成交量
		double	tick_OpenInterest[20][60];		//持仓量

		double	Day_CloseProfit[20];
		double	Day_CloseProfitA[20];
		double	Day_CloseProfitB[20];
		double	Day_TradeNumb[20];
	} stMDATA;
#endif
};

inline string ff2ss(double nums)
{
	stringstream ss;
	string str;
	ss << nums;
	ss >> str;
	char *chr = new char[str.length()];
	sprintf(chr, "%.0f", nums);
	string tt = chr;
	return tt;
}

#ifdef THOST_FTDCMDAPI_H

class MarketDataCollector :
	public CThostFtdcMdSpi
{
public:
	MarketDataCollector() {};
	MarketDataCollector(CThostFtdcMdApi* api);
	virtual ~MarketDataCollector();
private:
	int iInstrumentID = 1;
	int iResult = -1;
	// 交易时间
	TThostFtdcDateExprType	TradingDay;								//交易日期

	bool	JustRun = false;										//正在启动标志
	bool	CloseAll = false;										//收盘标志

	int		FirstVolume = 0;											//前一次成交量数据

	string	InstrumentID_name = "";	//缓存TICK合约名称

	int		Q_BarTime_1 = 0;		//缓存TICK时间戳：秒计算
	double	Q_BarTime_2 = 0;		//缓存TICK时间戳：0.145500

	int		Q_BarTime_1n[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//合约时间戳：秒计算
	int		Trade_times[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//开仓时间戳：秒计算

	char	LogFilePaths[80] = "";			//交易日志
	char	TickFileWritepaths[20][80] = { "", "", "", "", "", "", "", "", "", "" ,"", "", "", "", "", "", "", "", "", "" };	//TICK数据保存文件名次格式，合约名称_日期.txt
																																//											0		1			2		  3			4		 5		  6		   7		8			9		10		11		12		13		14		15		  16		17	   18		 19
	char	InstrumentID_n[20][10] = { "i1409", "jm1409", "j1409", "rb1410","rb1501", "TA409", "l1409","ru1409", "ru1501", "jd1409", "RM409", "m1409","y1501", "p1501","ag1506","ag1412","cu1408","cu1409","IF1409","IF1407" };//交易合约

	int		InstrumentID_En[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };	//交易使能 =1，会实盘下单
	int		InstrumentID_lots[20] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };	//开仓量

	double  InstrumentID_minmove[20] = { 1, 1, 1, 1, 1, 2, 5, 5, 5, 1, 1, 1, 2, 2, 1, 1, 10, 10, 0.2, 0.2, };	//最小变动价位

	double	Trade_Stopwin[20] = { 30, 30, 30, 20, 20, 30, 30, 120, 120, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 };			//单次开仓止赢点
	double	Trade_Stoploss[20] = { 30, 30, 30, 20, 20, 30, 30, 180, 180, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 };			//单次开仓止损点
	double	Trade_StopCloseProfit[20] = { 30, 30, 30, 10, 10, 30, 30, 110, 110, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 };	//>止损,限制新开
																																//tick数据
	bool	FristTick[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//收到当日第一个有效TICK标记
	bool	LastTick[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//收到当日最后一个有效TICK标记
	bool	ReceiveTick[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//TICK数据接收标记，暂未使用

	double	tick_data[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:基本信息

	double	tick_AskPrice1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
	double	tick_BidPrice1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
	double	tick_AskVolume1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
	double	tick_BidVolume1[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据
	double	tick_Volume[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//行情数据:保存60个TICK数据
	double	tick_OpenInterest[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//行情数据:保存60个TICK数据

	double	Sniffer_dataA[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
	double	Sniffer_dataB[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
	double	Sniffer_dataC[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
	double	Sniffer_dataD[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//监测数据
																										//----------------------
	double  Day_open[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//日K线数据开 
	double  Day_high[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//日K线数据高
	double  Day_low[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//日K线数据低
	double  Day_close[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//日K线数据收

	bool	MnKlinesig[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//分钟K线第一个TICK标记
	double  Mn_open[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//分钟K线数据开 
	double  Mn_high[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//分钟K线数据高
	double  Mn_low[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//分钟K线数据低
	double  Mn_close[20][60] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//分钟K线数据收
																										//-----------------------
	bool	SnifferSignalA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//指标策略运算标记
	bool	TradingSignalA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//下单标记
	bool	SnifferSignalB[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//指标策略运算标记
	bool	TradingSignalB[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//下单标记
																										//基本订单数据
	double	Trade_dataA[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
	double	Trade_dataB[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
	double	Trade_dataC[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
	double	Trade_dataD[20][10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//订单数据
																										//测试开平仓统计数据
	double	Trade_CloseProfit[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//平仓盈亏，测试策略用
	double	Trade_Closetimes[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//平仓次数，测试策略用
	double	Day_CloseProfit[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//当日平仓收益，测试策略用
	double	Day_CloseProfitA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//当日平仓收益，测试策略用
	double	Day_CloseProfitB[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };		//当日平仓收益，测试策略用
	double	Day_TradeNumb[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };			//开仓次数统计
public:
	// 行情接口
	int CtpMarketMainApi();
public:
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnFrontConnected() override;
	virtual void OnFrontDisconnected(int nReason) override;
	virtual void OnHeartBeatWarning(int nTimeLapse) override;
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;
private:
	int CtpMarketReqUserLogin();
	int CtpMarketSubscribe();
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
	int CtpMarketUnSubscribe(char *arrayOfConracts[], int sizeOfArray);
	int CtpMarketLogout(CThostFtdcUserLogoutField *req, int requestId);
};

#endif // THOST_FTDCMDAPI_H

class TradeChannel :
	CThostFtdcTraderSpi
{
public:
	TradeChannel() {};
	TradeChannel(CThostFtdcTraderApi* api);
	virtual ~TradeChannel();
private:
	int iResult = -1;
	time_t lOrderTime;
	time_t lOrderOkTime;
public:
	// 交易接口
	int CtpTradeMainApi();
	int getRtn();
public:
	virtual void OnFrontConnected() override;
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField * pRspUserLogin, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder) override;
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade) override;
	virtual void OnHeartBeatWarning(int nTimeLapse) override;
	virtual void OnFrontDisconnected(int nReason) override;
private:
	///用户登陆请求
	int CtpTrdReqUserLogin();
	///投资者结算结果确认
	void CtpTrdReqSettlementInfoConfirm();
	///请求查询合约
	void CtpTrdReqQryInstrument();
	///请求查询资金账户
	void CtpTrdReqQryTradingAccount();
	///请求查询投资者持仓
	void CtpTrdReqQryInvestorPosition();
	///报单录入请求
	void CtpTrdReqOrderInsert();
	///报单操作请求
	void CtpTrdReqOrderAction(CThostFtdcOrderField *pOrder);
	//是否收到成功响应
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
	//是否我的报单回报
	bool IsMyOrder(CThostFtdcOrderField *pOrder);
	//是否当前交易报单
	bool IsTradingOrder(CThostFtdcOrderField *pOrder);
};
