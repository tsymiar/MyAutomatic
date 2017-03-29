#include "DOSCout.h"

DOSCout::DOSCout(){}

void DOSCout::RedirectConsole()
{
	int nRet = 0;
	FILE* fp;
	AllocConsole();
	nRet = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	fp = _fdopen(nRet, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}

#ifdef _CONSOLE\
||_WINDOWS

BOOL WINAPI ConsoleHandler(DWORD msgType)
{
	int j;
	BOOL g_bExit = FALSE;
	/*HMENU hMenu = GetSystemMenu(GetConsoleWindow(), false);
	EnableMenuItem(hMenu, SC_CLOSE, MF_DISABLED | MF_BYCOMMAND);*/
	switch (msgType)
	{
	case CTRL_CLOSE_EVENT:
		std::cin >> j;
		printf("Close console window!\n");
		g_bExit = TRUE;
		break;
	default:
		break;
	}
	return g_bExit;
}

class __TRACE
{
	CString szFileAndLine;
public:
	__TRACE(LPCTSTR szFile, int nLien)
	{
		szFileAndLine.Format(_T("%s(%d): "), szFile, nLien);
	}

	void __cdecl operator()(LPCTSTR format, ...) const
	{
		TCHAR szInfo[1024] = { 0 };
		va_list arg; va_start(arg, format);
		_vstprintf_s(szInfo, 1024, format, arg);
		va_end(arg);
		OutputDebugString(szFileAndLine);
		OutputDebugString(szInfo);
	}
};
#ifdef _DEBUG
#define _TRACE __TRACE(_T(__FILE__), __LINE__)
#endif
#endif

void DOSCout::ExecuteConsole()
{
	//#pragma comment(linker,"/subsystem:\"Windows\" /entry:\"mainCRTStartup\"")
	FILE* stream;
	AllocConsole();
	SetConsoleTitle(
#ifdef QMyOglWdg_H
		"TextOut"
#else
		_T("TextOut")
#endif
	);
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	cout << "Press Esc to exit." << endl;
	DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND);
	//×èÈûÖ÷Ïß³Ì
	while (true)
	{
		if (_getch() == 27)
			ShowWindow(::FindWindow(NULL, _T("TextOut")), SW_HIDE);
			break;
	}
}

void DOSCout::ConsoleIOoverload()
{
	int hConHandle;
	HANDLE lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;
	// allocate a console for this app
	AllocConsole();
	SetConsoleTitle(_T("TextOut"));
	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
	// redirect unbuffered STDOUT to the console
	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((INT_PTR)lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w+t");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
	// redirect unbuffered STDIN to the console
	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((INT_PTR)lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "r+t");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);
	// redirect unbuffered STDERR to the console
	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((INT_PTR)lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);
	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog 
	// point to console as well
	std::ios::sync_with_stdio();
	cout << "Press Esc to exit." << endl;
	DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND);
	while (true)
	{
		if (_getch() == 27)
			ShowWindow(::FindWindow(NULL, _T("TextOut")), SW_HIDE);
		break;
	}
	//if (SetWindowsHookEx(WH_CALLWNDPROC, ConsoleHandler, 0, GetCurrentThreadId()) == NULL)
	//{
	//  std::cout << "Error!\n\t--Console Created Failed." << std::endl;
	//	FreeConsole();
	//}
}

DOSCout::~DOSCout(){}
