#pragma once
#include	<cstring>
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"

class MarketDataCollector :
	public CThostFtdcMdSpi
{
public:
	MarketDataCollector() {};
	MarketDataCollector(CThostFtdcMdApi* api);
	virtual ~MarketDataCollector();
private:
	CThostFtdcMdApi *pUserApi = nullptr;
	int requestId = 0;
	int ret = -1;
public:
	// 行情接口
	int CtpMarketInit(const char* mdflowpath/* = "*.con"*/, char* mdfront = "tcp://127.0.0.1:17001", bool bIsMulticast = false);
	int CtpMarketLogin(char* BrokerID = "2030", char* UserID = "023526", char*PassWord = "*******");
	int CtpMarketSubscribe(char* arrayOfConracts[], int sizeOfArray);
	int CtpMarketUnSubscribe(char *arrayOfConracts[], int sizeOfArray);
	int CtpMarketLogout(CThostFtdcUserLogoutField *req, int requestId);
};

class TradeChannel :
	CThostFtdcTraderSpi
{
public:
	TradeChannel() {};
	TradeChannel(CThostFtdcTraderApi* api);
	virtual ~TradeChannel();
private:
	CThostFtdcTraderApi *pUserApi = nullptr;
	int requestId = 0;
	int ret = -1;
public:
	// 交易接口
	int CtpTradeInit(const char* tdflowpath, char* tdfront);
	int CtpTradeLogin(char* BrokerID, char* UserID, char*PassWord, TThostFtdcAuthCodeType AuthCode = nullptr);
};
