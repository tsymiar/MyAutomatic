#ifndef _CTPCLIENT_H_
#define _CTPCLIENT_H_

#include "../MyOglDrawDlg.h"

class CTPclient
{
public:
	OGLKview Ogl;
	CTPclient();
	virtual ~CTPclient();
	static unsigned int __stdcall SimpleClient(void* P);
	static unsigned int __stdcall TradeMarket(void* P);
};

#endif
