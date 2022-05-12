#include "ChannelCollector.h"

extern struct st_TThostFtdc STFTDC;
extern CThostFtdcTraderApi *TRDAPI;

#ifdef THOST_FTDCMDAPI_H

MarketDataCollector::MarketDataCollector(CThostFtdcMdApi * api) {}
MarketDataCollector::~MarketDataCollector() {}

int MarketDataCollector::CtpMarketMainApi()
{
    CThostFtdcMdApi *api =
        CThostFtdcMdApi::CreateFtdcMdApi(STFTDC.FLOW_PATH, true, STFTDC.ISMULTICAST);
    MarketDataCollector mdCollector(api);
    api->RegisterSpi(&mdCollector);
    api->RegisterFront(STFTDC.FRONT_ADDR);
    api->Init();
    api->Join();
    return 0;
}

void MarketDataCollector::OnRspError(CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << __FUNCTION__ << endl;
    IsErrorRspInfo(pRspInfo);
}

void MarketDataCollector::OnFrontConnected()
{
    cerr << "--->>> " << "行情登录中..." << endl;
    CtpMarketReqUserLogin();
}

void MarketDataCollector::OnFrontDisconnected(int nReason)
{
    cerr << "--->>> " << __FUNCTION__ << endl;
    cerr << "--->>> Reason = " << nReason << endl;
}

void MarketDataCollector::OnHeartBeatWarning(int nTimeLapse)
{
    cerr << "--->>> " << __FUNCTION__ << endl;
    cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

/*
?  0: 发送成功
? -1: 因网络原因发送失败
? -2: 未处理请求队列总数量超限。
? -3: 每秒发送请求数量超限。
*/
int MarketDataCollector::CtpMarketReqUserLogin()
{
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    strcpy_s(req.UserID, STFTDC.USER_ID);
    strcpy_s(req.Password, STFTDC.PASSWORD);
    iResult = TRDAPI->ReqUserLogin(&req, ++STFTDC.iReqID);
    cerr << "--->>> 发送用户登录请求: ", iResult == 0 ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "") << endl;
    void onRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    return iResult;
}

void MarketDataCollector::OnRspUserLogin(CThostFtdcRspUserLoginField * pRspUserLogin, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << __FUNCTION__ << endl;

    if (IsErrorRspInfo(pRspInfo))
    {
        cerr << "--->>> 行情登录错误: " << pRspInfo->ErrorID << pRspInfo->ErrorMsg << endl;
    }
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///获取当前交易日
        cerr << "--->>> 获取当前交易日 = " << STFTDC.MdApi->GetTradingDay() << endl;
        // 请求订阅行情
        CtpMarketSubscribe();
    }
}

void MarketDataCollector::OnRspSubMarketData(CThostFtdcSpecificInstrumentField * pSpecificInstrument, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "订阅行情品种：" << pSpecificInstrument->InstrumentID << endl;
}

void MarketDataCollector::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField * pSpecificInstrument, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << __FUNCTION__ << endl;
}

void MarketDataCollector::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField * pDepthMarketData)
{
    //cerr << "--->>> " << __FUNCTION__ << endl;
    ///数据缓存
    double GetLocalTimeMs1();
    void WriteConfiguration(char *filepaths);
    void Count_Day_BuySellpos(int i);

    char names[10] = "";
    char times[10] = "";

    strcpy_s(names, pDepthMarketData->InstrumentID);
    strcpy_s(times, pDepthMarketData->UpdateTime);
    strcpy(TradingDay, pDepthMarketData->TradingDay);

    InstrumentID_name = names;
    string str1 = times;
    string str2 = times;
    string str3 = times;

    str1 = str1.substr(0, 2);        //
    str2 = str2.substr(3, 2);        //
    str3 = str3.substr(6, 2);        //
    int hours = atoi(str1.c_str());
    int minutes = atoi(str2.c_str());
    int seconds = atoi(str3.c_str());
    int Millisecs = pDepthMarketData->UpdateMillisec;

    double NewPrice = pDepthMarketData->LastPrice;

    Q_BarTime_1 = hours * 60 * 60 + minutes * 60 + seconds;                        //时间采用秒计
    Q_BarTime_2 = (1 / 10e1)*hours + (1 / 10e3)*minutes + (1 / 10e5)*seconds;    //时间格式0.145100 = 14：51：00



    for (int i = 0; i < 20; i++)
    {
        if (InstrumentID_name == InstrumentID_n[i])
        {
            ReceiveTick[i] = true;

            Q_BarTime_1n[i] = Q_BarTime_1;

            tick_data[i][0] = 2;        //设置标志位
            tick_data[i][1] = atof(pDepthMarketData->TradingDay);
            tick_data[i][2] = Q_BarTime_2;//pDepthMarketData->UpdateTime;
            tick_data[i][3] = pDepthMarketData->UpdateMillisec;

            tick_data[i][4] = pDepthMarketData->LastPrice;
            tick_data[i][5] = pDepthMarketData->AveragePrice;
            tick_data[i][6] = pDepthMarketData->HighestPrice;
            tick_data[i][7] = pDepthMarketData->LowestPrice;

            //***************************************************    

            bool check0 = (tick_data[i][2] > 0.0910 && tick_data[i][2] < 0.0915 && i>17) || (tick_data[i][2]<0.1518 && tick_data[i][2]>0.0914 && !FristTick[i] && i > 17);
            bool check1 = (tick_data[i][2] > 0.0856 && tick_data[i][2] < 0.0900 && i <= 17) || (tick_data[i][2]<0.1505 && tick_data[i][2]>0.0859 && !FristTick[i] && i <= 17);
            bool check2 = (tick_data[i][2] > 0.2056 && tick_data[i][2] < 0.2100 && i <= 17) || ((tick_data[i][2]<0.0235 || tick_data[i][2]>0.2059) && !FristTick[i] && i <= 17);

            if (STFTDC.stMDATA.MdMode && (check0 || check1 || check2))
            {
                MnKlinesig[i] = true;
                Day_open[i][0] = pDepthMarketData->OpenPrice;

                Mn_open[i][1] = Day_open[i][0];
                Mn_high[i][1] = Day_open[i][0];
                Mn_low[i][1] = Day_open[i][0];
                Mn_close[i][1] = Day_open[i][0];

                Mn_open[i][0] = Day_open[i][0];
                Mn_high[i][0] = Day_open[i][0];
                Mn_low[i][0] = Day_open[i][0];
                Mn_close[i][0] = Day_open[i][0];

                tick_data[i][8] = pDepthMarketData->UpperLimitPrice;    //涨停价
                tick_data[i][9] = pDepthMarketData->LowerLimitPrice;    //跌停价                    

                Day_CloseProfit[i] = 0;
                Day_CloseProfitA[i] = 0;
                Day_CloseProfitB[i] = 0;

                Day_TradeNumb[i] = 0;

            }

            //***************************************************    
            for (int j = 59; j > 0; j--)
            {
                tick_AskPrice1[i][j] = tick_AskPrice1[i][j - 1];
                tick_BidPrice1[i][j] = tick_BidPrice1[i][j - 1];
                tick_AskVolume1[i][j] = tick_AskVolume1[i][j - 1];
                tick_BidVolume1[i][j] = tick_BidVolume1[i][j - 1];
                tick_Volume[i][j] = tick_Volume[i][j - 1];
                tick_OpenInterest[i][j] = tick_OpenInterest[i][j - 1];
            }

            //实盘开盘前登录，交易所此处插入的最大值，需屏蔽处理
            if ((tick_data[i][2] > 0.0250 && tick_data[i][2] < 0.0850) || (tick_data[i][2] > 0.1550 && tick_data[i][2] < 0.2050))
                //if (AskPrice1t>99999 || BidPrice1t>99999)
            {
                tick_AskPrice1[i][0] = 0;
                tick_BidPrice1[i][0] = 0;
            }
            else
            {
                tick_AskPrice1[i][0] = pDepthMarketData->AskPrice1;
                tick_BidPrice1[i][0] = pDepthMarketData->BidPrice1;
            }

            tick_AskVolume1[i][0] = pDepthMarketData->AskVolume1;
            tick_BidVolume1[i][0] = pDepthMarketData->BidVolume1;
            tick_Volume[i][0] = pDepthMarketData->Volume;
            tick_OpenInterest[i][0] = pDepthMarketData->OpenInterest;

            //***************************************************    

            bool Timemore0 = tick_data[i][2] != 0.0859 && tick_data[i][2] != 0.0900 && tick_data[i][2] != 0.1015 && tick_data[i][2] != 0.1130 && tick_data[i][2] != 0.1500;
            bool Timemore1 = tick_data[i][2] != 0.2059 && tick_data[i][2] != 0.2100 && tick_data[i][2] != 0.0230;
            bool Timemore2 = (tick_data[i][2] > 0.0900 && tick_data[i][2] < 0.1500) || tick_data[i][2] > 0.2100 || tick_data[i][2] < 0.0230;

            if (Timemore0 && Timemore1  && Timemore2 && seconds >= 0 && seconds<40 && tick_Volume[i][0]>tick_Volume[i][1] && MnKlinesig[i] == false)
            {
                MnKlinesig[i] = true;

                Mn_open[i][1] = Mn_open[i][0];
                Mn_high[i][1] = Mn_high[i][0];
                Mn_low[i][1] = Mn_low[i][0];
                Mn_close[i][1] = Mn_close[i][0];

                Mn_open[i][0] = NewPrice;
                Mn_high[i][0] = NewPrice;
                Mn_low[i][0] = NewPrice;
                Mn_close[i][0] = NewPrice;
                //打印K线线数据
                //cerr << "--->>> " << InstrumentID_n[i] << "_" <<ff2ss(tick_data[i][1])<< "_" << tick_data[i][2]<< "_" << Mn_open[i][1]<< "_" << Mn_high[i][1]<< "_" << Mn_low[i][1]<< "_" << Mn_close[i][1] << endl;
            }
            else
            {
                Mn_high[i][0] = max(Mn_high[i][0], NewPrice);
                Mn_low[i][0] = min(Mn_low[i][0], NewPrice);
                Mn_close[i][0] = NewPrice;
            }

            if (seconds > 45 && seconds < 55 && MnKlinesig[i] == true)
            {
                MnKlinesig[i] = false;
            }
            //***************************************************

            //tick_data[i][0]= 0;        //设置标志位

            if (TickFileWritepaths[i][0] == '\0')
            {
                tick_data[i][8] = pDepthMarketData->UpperLimitPrice;
                tick_data[i][9] = pDepthMarketData->LowerLimitPrice;

                strcpy_s(TickFileWritepaths[i], "./TickData/");
                strcat_s(TickFileWritepaths[i], pDepthMarketData->InstrumentID);
                strcat_s(TickFileWritepaths[i], "_");
                strcat_s(TickFileWritepaths[i], TradingDay);
                strcat_s(TickFileWritepaths[i], ".txt");

                //检查文件是否存在，是否需要新建文本文件
                ifstream inf;
                ofstream ouf;
                inf.open(TickFileWritepaths[i], ios::out);
            }

            //记录TICK数据
            ofstream o_file(TickFileWritepaths[i], ios::app);

            if (STFTDC.stMDATA.MdMode && (check0 || check1 || check2))
            {
                o_file << ff2ss(tick_data[i][1]) << "\t" << tick_data[i][2] << "\t" << Millisecs << "\t" << tick_AskPrice1[i][0] << "\t" << tick_AskVolume1[i][0] << "\t" << tick_BidPrice1[i][0] << "\t" << tick_BidVolume1[i][0] << "\t" << tick_data[i][4] << "\t" << ff2ss(tick_Volume[i][0]) << "\t" << ff2ss(tick_OpenInterest[i][0]) << "\t" << Day_open[i][2] << "\t" << Day_high[i][2] << "\t" << Day_low[i][2] << "\t" << Day_close[i][2] << "\t" << Day_open[i][1] << "\t" << Day_high[i][1] << "\t" << Day_low[i][1] << "\t" << Day_close[i][1] << "\t" << tick_data[i][8] << "\t" << tick_data[i][9] << endl;
            }
            else
            {
                o_file << ff2ss(tick_data[i][1]) << "\t" << tick_data[i][2] << "\t" << Millisecs << "\t" << tick_AskPrice1[i][0] << "\t" << tick_AskVolume1[i][0] << "\t" << tick_BidPrice1[i][0] << "\t" << tick_BidVolume1[i][0] << "\t" << tick_data[i][4] << "\t" << ff2ss(tick_Volume[i][0]) << "\t" << ff2ss(tick_OpenInterest[i][0]) << endl; //将内容写入到文本文件中
            }
            o_file.close();                        //关闭文件


            if (tick_data[i][2] > 0.145950 && (tick_Volume[i][0] - tick_Volume[i][1]) < 0.01 && (tick_OpenInterest[i][0] - tick_OpenInterest[i][1]) < 0.01)// && LastTick[i]==false)
            {
                //LastTick[i]=true;

                Day_open[i][0] = pDepthMarketData->OpenPrice;
                Day_high[i][0] = pDepthMarketData->HighestPrice;
                Day_low[i][0] = pDepthMarketData->LowestPrice;
                Day_close[i][0] = pDepthMarketData->LastPrice;
                //cerr << "--->>> " <<"WriteConfiguration!" << endl;
                //WriteConfiguration("./AutoTrader.dat");
            }

            if ((tick_data[i][2] > 0.0913 && i > 17) || (tick_data[i][2] > 0.0858 && i <= 17))
            {
                FristTick[i] = true;
            }

            ReceiveTick[i] = false;
            tick_data[i][0] = 1;        //设置标志位

        }

    }

    //cerr << "--->>> " << InstrumentID_name << "Tick is OK!" << endl;
    //cerr << "--->>> " << "Time is:" <<setprecision(9)<< GetLocalTimeMs1() << endl;
}

// 第一个参数是一个包含所有要订阅的合约的数组
// 第二个参数是该数组的长度
int MarketDataCollector::CtpMarketSubscribe()
{
    Sleep(1000);
    iResult = STFTDC.MdApi->SubscribeMarketData(STFTDC.stMDATA.Instruments, iInstrumentID);
    (cerr << "--->>> 发送行情订阅请求: ", iResult == 0 ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
    return iResult;
}

bool MarketDataCollector::IsErrorRspInfo(CThostFtdcRspInfoField * pRspInfo)
{
    // 如果ErrorID != 0, 说明收到了错误的响应
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        cerr << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
    return bResult;
}

int MarketDataCollector::CtpMarketUnSubscribe(char *arrayOfConracts[], int sizeOfArray)
{
    return STFTDC.MdApi->UnSubscribeMarketData(arrayOfConracts, sizeOfArray);
}

int MarketDataCollector::CtpMarketLogout(CThostFtdcUserLogoutField *req, int iReqID)
{
    return STFTDC.MdApi->ReqUserLogout(req, iReqID);
}

#endif

TradeChannel::TradeChannel(CThostFtdcTraderApi* api) : lOrderTime(0), lOrderOkTime(0)
{
}

TradeChannel::~TradeChannel()
{
}

/*
订阅模式
? THOST_TERT_RESTART：    接收所有交易所当日曾发送过的以及之后可能会发送的所有该类消息。
? THOST_TERT_RESUME：    接收客户端上次断开连接后交易所曾发送过的以及之后可能会发送的所有该类消息。
? THOST_TERT_QUICK：        接收客户端登录之后交易所可能会发送的所有该类消息。
*/
int TradeChannel::CtpTradeMainApi()
{
    CThostFtdcTraderApi *api =
        CThostFtdcTraderApi::CreateFtdcTraderApi(STFTDC.FLOW_PATH);
    TradeChannel tdChnl(TRDAPI = api);
    api->RegisterSpi(&tdChnl);
    api->RegisterFront(STFTDC.FRONT_ADDR);
    api->SubscribePrivateTopic(THOST_TERT_QUICK);
    api->SubscribePublicTopic(THOST_TERT_QUICK);
    api->Init();
    api->Join();
    return 0;
}

int TradeChannel::getRtn()
{
    return iResult;
}

void TradeChannel::OnFrontConnected()
{
    cerr << "--->>> " << "OnFrontConnected" << endl;
    ///用户登录请求
    CtpTrdReqUserLogin();
}

bool TradeChannel::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    // 如果ErrorID != 0, 说明收到了错误的响应
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        cerr << "--->>> ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg-[" << pRspInfo->ErrorMsg << "]" << endl;
    return bResult;
}

/*
身份认证功能是否启用在期货公司的业务人员使用的结算平台上是可以进行配置的。期货公司可以选择关闭身份认证功能
则客户端可不必进行身份认证。否则期货公司需要在结算平台上维护该客户端程序的认证码（AuthCode）
请求进行身份认证使用的函数接口为 ReqAuthenticate（请求身份认证）
和 OnRspAuthenticate（服务端返回的身份认证的响应）。
*/
int TradeChannel::CtpTrdReqUserLogin()
{
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    strcpy_s(req.UserID, STFTDC.INVESTOR_ID);
    strcpy_s(req.Password, STFTDC.PASSWORD);
    if (STFTDC.AUTHCODE[0] != '\0')
    {
        CThostFtdcReqAuthenticateField authReq;
        memset(&authReq, 0, sizeof(authReq));
        strcpy_s(authReq.BrokerID, STFTDC.BROKER_ID);
        strcpy_s(authReq.UserID, STFTDC.INVESTOR_ID);
        strcpy_s(authReq.AuthCode, STFTDC.AUTHCODE);
        if ((iResult = TRDAPI->ReqAuthenticate(&authReq, ++STFTDC.iReqID)) != 0)
            return iResult;
    }
    iResult = TRDAPI->ReqUserLogin(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 发送用户登录请求: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
    return iResult;
}

void TradeChannel::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspUserLogin" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        // 保存会话参数
        STFTDC.FRONT_ID = pRspUserLogin->FrontID;
        STFTDC.SESSION_ID = pRspUserLogin->SessionID;
        int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
        iNextOrderRef++;
        sprintf_s(STFTDC.ORDER_REF, "%d", iNextOrderRef);
        ///获取当前交易日
        cerr << "--->>> 获取当前交易日 = " << TRDAPI->GetTradingDay() << endl;
        ///投资者结算结果确认
        CtpTrdReqSettlementInfoConfirm();
    }
}

void TradeChannel::CtpTrdReqSettlementInfoConfirm()
{
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    strcpy_s(req.InvestorID, STFTDC.INVESTOR_ID);
    int iResult = TRDAPI->ReqSettlementInfoConfirm(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 投资者结算结果确认: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
}

void TradeChannel::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspSettlementInfoConfirm" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///请求查询合约
        CtpTrdReqQryInstrument();
    }
}

void TradeChannel::CtpTrdReqQryInstrument()
{
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    strcpy_s(req.InstrumentID, STFTDC.INSTRUMENT_ID);
    int iResult = TRDAPI->ReqQryInstrument(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 请求查询合约: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
}

void TradeChannel::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryInstrument" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///请求查询合约
        CtpTrdReqQryTradingAccount();
    }
}

void TradeChannel::CtpTrdReqQryTradingAccount()
{
    CThostFtdcQryTradingAccountField req;
    memset(&req, 0, sizeof(req));
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    strcpy_s(req.InvestorID, STFTDC.INVESTOR_ID);
    int iResult = TRDAPI->ReqQryTradingAccount(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 请求查询资金账户: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
}

void TradeChannel::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryTradingAccount" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///请求查询投资者持仓
        CtpTrdReqQryInvestorPosition();
    }
}

void TradeChannel::CtpTrdReqQryInvestorPosition()
{
    CThostFtdcQryInvestorPositionField req;
    memset(&req, 0, sizeof(req));
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    strcpy_s(req.InvestorID, STFTDC.INVESTOR_ID);
    strcpy_s(req.InstrumentID, STFTDC.INSTRUMENT_ID);
    int iResult = TRDAPI->ReqQryInvestorPosition(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 请求查询投资者持仓: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
}

void TradeChannel::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryInvestorPosition" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///报单录入请求
        CtpTrdReqOrderInsert();
    }
}

void TradeChannel::CtpTrdReqOrderInsert()
{
    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy_s(req.BrokerID, STFTDC.BROKER_ID);
    ///投资者代码
    strcpy_s(req.InvestorID, STFTDC.INVESTOR_ID);
    ///合约代码
    strcpy_s(req.InstrumentID, STFTDC.INSTRUMENT_ID);
    ///报单引用
    strcpy_s(req.OrderRef, STFTDC.ORDER_REF);
    ///用户代码
    //    TThostFtdcUserIDType    UserID;
    ///报单价格条件: 限价
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向: 
    req.Direction = STFTDC.DIRECTION;
    ///组合开平标志: 开仓
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ///组合投机套保标志
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ///价格
    req.LimitPrice = STFTDC.LIMIT_PRICE;
    ///数量: 1
    req.VolumeTotalOriginal = 1;
    ///有效期类型: 当日有效
    req.TimeCondition = THOST_FTDC_TC_GFD;
    ///GTD日期
    //    TThostFtdcDateType    GTDDate;
    ///成交量类型: 任何数量
    req.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    req.MinVolume = 1;
    ///触发条件: 立即
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///止损价
    //    TThostFtdcPriceType    StopPrice;
    ///强平原因: 非强平
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    req.IsAutoSuspend = 0;
    ///业务单元
    //    TThostFtdcBusinessUnitType    BusinessUnit;
    ///请求编号
    //    TThostFtdcRequestIDType    RequestID;
    ///用户强评标志: 否
    req.UserForceClose = 0;

    lOrderTime = time(NULL);
    int iResult = TRDAPI->ReqOrderInsert(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 报单录入请求: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
}

void TradeChannel::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspOrderInsert" << endl;
    IsErrorRspInfo(pRspInfo);
}

void TradeChannel::CtpTrdReqOrderAction(CThostFtdcOrderField *pOrder)
{
    static bool ORDER_ACTION_SENT = false;        //是否发送了报单
    if (ORDER_ACTION_SENT)
        return;

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy_s(req.BrokerID, pOrder->BrokerID);
    ///投资者代码
    strcpy_s(req.InvestorID, pOrder->InvestorID);
    ///报单操作引用
    //    TThostFtdcOrderActionRefType    OrderActionRef;
    ///报单引用
    strcpy_s(req.OrderRef, pOrder->OrderRef);
    ///请求编号
    //    TThostFtdcRequestIDType    RequestID;
    ///前置编号
    req.FrontID = STFTDC.FRONT_ID;
    ///会话编号
    req.SessionID = STFTDC.SESSION_ID;
    ///交易所代码
    //    TThostFtdcExchangeIDType    ExchangeID;
    ///报单编号
    //    TThostFtdcOrderSysIDType    OrderSysID;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///价格
    //    TThostFtdcPriceType    LimitPrice;
    ///数量变化
    //    TThostFtdcVolumeType    VolumeChange;
    ///用户代码
    //    TThostFtdcUserIDType    UserID;
    ///合约代码
    strcpy_s(req.InstrumentID, pOrder->InstrumentID);
    lOrderTime = time(NULL);
    int iResult = TRDAPI->ReqOrderAction(&req, ++STFTDC.iReqID);
    ((cerr << "--->>> 报单操作请求: ", iResult == 0) ? cerr << "成功" : cerr << "失败 [" << iResult << (iResult != 0 ? "]" : "")) << endl;
    ORDER_ACTION_SENT = true;
}

void TradeChannel::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspOrderAction" << endl;
    IsErrorRspInfo(pRspInfo);
}

///报单通知
void TradeChannel::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    cerr << "--->>> " << "OnRtnOrder" << endl;
    lOrderOkTime = time(NULL);
    time_t lTime = lOrderOkTime - lOrderTime;
    cerr << "--->>> 报单到报单通知的时间差 = " << lTime << endl;
    if (IsMyOrder(pOrder))
    {
        if (IsTradingOrder(pOrder))
        {
            //ReqOrderAction(pOrder);
        }
        else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
            cerr << "--->>> 撤单成功" << endl;
    }
}

///成交通知
void TradeChannel::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    cerr << "--->>> " << "OnRtnTrade" << endl;
}

void TradeChannel::OnFrontDisconnected(int nReason)
{
    cerr << "--->>> " << "OnFrontDisconnected" << endl;
    cerr << "--->>> Reason = " << nReason << endl;
}

void TradeChannel::OnHeartBeatWarning(int nTimeLapse)
{
    cerr << "--->>> " << "OnHeartBeatWarning" << endl;
    cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void TradeChannel::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspError" << endl;
    IsErrorRspInfo(pRspInfo);
}

bool TradeChannel::IsMyOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->FrontID == STFTDC.FRONT_ID) &&
        (pOrder->SessionID == STFTDC.SESSION_ID) &&
        (strcmp(pOrder->OrderRef, STFTDC.ORDER_REF) == 0));
}

bool TradeChannel::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
        (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
        (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}
