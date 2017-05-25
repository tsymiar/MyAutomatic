#ifndef _CTPDEV_H_
#define _CTPDEV_H_

#include "MYGL/OGLKview.h"
#include "ChannelCollector.h"
#include "../MyOglDrawDlg.h"
#include "dos\DOSCout.h"

class CTPdev
{
public:
	OGLKview Ogl;
	CTPdev();
	virtual ~CTPdev();
	static unsigned int __stdcall SimpleClient(void* P);
	static unsigned int __stdcall TradeMarket(void* P);
};
#endif
