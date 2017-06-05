#ifndef DOS_COUT_H_
#define DOS_COUT_H_

#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <tchar.h>
#include <conio.h>
#include <cstdio>
#include <iostream>

#if !defined(_DEBUG)
#define _TRACE(...) __noop 
#endif

class DOSCout
{
public:
	DOSCout();
	virtual ~DOSCout();
	void ExecuteConsole();
	void RedirectConsole();
	void ConsoleIOoverload();
private:
	static const WORD CONSOLE_LINES = 500;
};

#endif // !DOS_COUT_H_