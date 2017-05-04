#include "ChannelCollector.h"


MarketDataCollector::MarketDataCollector(CThostFtdcMdApi * api)
{
}


MarketDataCollector::~MarketDataCollector()
{
}

int MarketDataCollector::CtpMarketInit(const char* mdflowpath, char* mdfront, bool bIsMulticast)
{
	CThostFtdcMdApi *api =
		CThostFtdcMdApi::CreateFtdcMdApi(mdflowpath, true, bIsMulticast);
	MarketDataCollector mdCollector(this->pUserApi = api);
	api->RegisterSpi(&mdCollector);
	api->RegisterFront(mdfront);
	api->Init();
	api->Join();
	return 0;
}

/*
参数：
BrokerID 期货公司的会员号。
UserID 投资者在该期货公司的客户号。
Password 该投资者密码。
返回：
  0: 发送成功
 -1: 因网络原因发送失败
 -2: 未处理请求队列总数量超限。
 -3: 每秒发送请求数量超限。
*/
int MarketDataCollector::CtpMarketLogin(char* BrokerID, char* UserID, char*PassWord)
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BrokerID);
	strcpy(req.UserID, UserID);
	strcpy(req.Password, PassWord);
	return ret = pUserApi->ReqUserLogin(&req, ++requestId);
	void onRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
}

// 第一个参数是一个包含所有要订阅的合约的数组
// 第二个参数是该数组的长度

int MarketDataCollector::CtpMarketSubscribe(char* arrayOfConracts[], int sizeOfArray)
{
	if (ret == 0)
		return pUserApi->SubscribeMarketData(arrayOfConracts, sizeOfArray);
	else
		return ret;
}

int MarketDataCollector::CtpMarketUnSubscribe(char *arrayOfConracts[], int sizeOfArray)
{
	return pUserApi->UnSubscribeMarketData(arrayOfConracts, sizeOfArray);
}

int MarketDataCollector::CtpMarketLogout(CThostFtdcUserLogoutField *req, int requestId)
{
	return pUserApi->ReqUserLogout(req, requestId);
}

TradeChannel::TradeChannel(CThostFtdcTraderApi * api)
{
}

TradeChannel::~TradeChannel()
{
}
/*
	订阅模式
	 THOST_TERT_RESTART：	接收所有交易所当日曾发送过的以及之后可能会发送的所有该类消息。
	 THOST_TERT_RESUME：	接收客户端上次断开连接后交易所曾发送过的以及之后可能会发送的所有该类消息。
	 THOST_TERT_QUICK：		接收客户端登录之后交易所可能会发送的所有该类消息。
*/
int TradeChannel::CtpTradeInit(const char* tdflowpath, char* tdfront)
{
	CThostFtdcTraderApi *api =
		CThostFtdcTraderApi::CreateFtdcTraderApi(tdflowpath);
	TradeChannel tdChnl(api);
	api->RegisterSpi(&tdChnl);
	api->RegisterFront(tdfront);
	api->SubscribePrivateTopic(THOST_TERT_QUICK);
	api->SubscribePublicTopic(THOST_TERT_QUICK);
	api->Init();
	api->Join();
	return 0;
}

/*
	身份认证功能是否启用在期货公司的业务人员使用的结算平台上是可以进行配置的。期货公司可以选择关闭身份认证功能
	则客户端可不必进行身份认证。否则期货公司需要在结算平台上维护该客户端程序的认证码（AuthCode）
	请求进行身份认证使用的函数接口为 ReqAuthenticate（请求身份认证）
	和 OnRspAuthenticate（服务端返回的身份认证的响应）。
*/
int TradeChannel::CtpTradeLogin(char* BrokerID, char* UserID, char*PassWord, TThostFtdcAuthCodeType AuthCode)
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BrokerID);
	strcpy(req.UserID, UserID);
	strcpy(req.Password, PassWord);
	if (AuthCode != nullptr)
	{
		CThostFtdcReqAuthenticateField authReq;
		memset(&authReq, 0, sizeof(authReq));
		strcpy(authReq.BrokerID, BrokerID);
		strcpy(authReq.UserID, UserID);
		strcpy(authReq.AuthCode, AuthCode);
		if (ret = pUserApi->ReqAuthenticate(&authReq, ++requestId) != 0)
			return ret;
	}
	return ret = pUserApi->ReqUserLogin(&req, ++requestId);
}