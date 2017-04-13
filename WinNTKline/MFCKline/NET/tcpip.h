#ifndef _TCPIP_H_
#define _TCPIP_H_

#include "MGL/OGLKview.h"

class TCPIP
{
public:
	TCPIP();
	virtual ~TCPIP();
	static unsigned int __stdcall SimpleClient(void* P);
private:
	OGLKview Ogl;
};
#endif
