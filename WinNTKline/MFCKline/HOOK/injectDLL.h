#include <cstdio>
#undef   UNICODE 
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h> 
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#ifdef HOOK_EXPORTS
#define IDM_FUCK     3333
#else
#pragma comment(lib, "./HOOK/hook.lib")
#endif
#define INJECT_PROCESS_NAME    "MFCKline.exe" //target

typedef WCHAR WPATH[MAX_PATH];
typedef DWORD64(WINAPI *PFNTCREATETHREADEX)
(
	PHANDLE                 ThreadHandle,
	ACCESS_MASK             DesiredAccess,
	LPVOID                  ObjectAttributes,
	HANDLE                  ProcessHandle,
	LPTHREAD_START_ROUTINE  lpStartAddress,
	LPVOID                  lpParameter,
	BOOL                    CreateSuspended,
	DWORD64                   dwStackSize,
	DWORD64                   dw1,
	DWORD64                   dw2,
	LPVOID                  Unknown
	);

extern"C"
{
	int injectDll();
	int ExitWindow(HWND hWndParent);
	int __declspec(dllexport) ShowDialog();
}
