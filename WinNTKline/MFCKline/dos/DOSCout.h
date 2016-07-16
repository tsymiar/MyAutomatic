#ifndef DOS_COUT_H_
#define DOS_COUT_H_

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <conio.h>

#pragma warning (disable:4067)
using namespace std;

class DOSCout
{
public:
	DOSCout();
	virtual ~DOSCout();
	void OpenConsole();
	void ConsoleIOoverload();
private:
	static const WORD CONSOLE_LINES = 500;
};

#endif // !DOS_COUT_H_