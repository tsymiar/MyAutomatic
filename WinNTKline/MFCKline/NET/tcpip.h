#pragma once
#include "OGL/OGLKview.h"

class tcpip
{
public:
	tcpip();
	virtual ~tcpip();
	static unsigned int __stdcall ClientThread(void* pParam);
private:
	OGLKview Ogl;
};

