#ifndef _TCPIP_H_
#define _TCPIP_H_

#include "MGL/OGLKview.h"

class TCPIP
{
public:
	TCPIP();
	virtual ~TCPIP();
	static unsigned int __stdcall ClientThread(void* pParam);
private:
	OGLKview Ogl;
};
#endif
