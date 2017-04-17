#ifndef _CTPDEV_H_
#define _CTPDEV_H_

#include "MGL/OGLKview.h"

class CTPdev
{
public:
	CTPdev();
	virtual ~CTPdev();
	static unsigned int __stdcall SimpleClient(void* P);
private:
	OGLKview Ogl;
};
#endif
