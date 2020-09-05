#include "stdafx.h"
#include "injectDLL.h"

//全局变量声明 
HINSTANCE parent_hInst;
HWND child_hWnd, parent_hWnd;
HANDLE   hProcessSnap = NULL;     //进程快照句柄 
DWORD64   dwRemoteProcessId;       //目标进程ID 
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

void ShowError()
{
    TCHAR* buffer = NULL;
    DWORD dwErrorCode = ::GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrorCode, 0, (LPTSTR)&buffer, 0, NULL);
}

void   GetWorkPath(TCHAR   szPath[], int   nSize)
{
    GetModuleFileName(NULL, szPath, nSize);
    char *p = strrchr(szPath, '\\');
    *p = 0;
}

BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.  
    //Use GetModuleHandle to get a handle to the DLL that contains the function  
    //and GetProcAddress to get a pointer to the function if available.  

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            //handle error  
        }
    }
    return bIsWow64;
}
//--------------------------------------------------------------------- 
//操作系统版本判断
BOOL IsVistaOrLater()
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    if (osvi.dwMajorVersion >= 6)
        return TRUE;
    return FALSE;
}
//--------------------------------------------------------------------- 
//OD跟踪，发现最后调用的是NtCreateThreadEx,所以这里手动调用
HANDLE MyCreateRemoteThread(HANDLE hProcess, LPTHREAD_START_ROUTINE pThreadProc, LPVOID pRemoteBuf)
{
    HANDLE      hThread = NULL;
    FARPROC     pFunc = NULL;
    if (IsVistaOrLater())    // Vista, 7, Server2008  
    {
        pFunc = GetProcAddress(GetModuleHandle("./hook.dll"), "NtCreateThreadEx");
        if (pFunc == NULL)
        {
            // AfxMessageBox("MyCreateRemoteThread() : GetProcAddress(\"NtCreateThreadEx\") 调用失败！错误代码: [%d]/n", GetLastError());
            return FALSE;
        }
        ((PFNTCREATETHREADEX)pFunc)(
            &hThread,
            0x1FFFFF,
            NULL,
            hProcess,
            pThreadProc,
            pRemoteBuf,
            FALSE,
            NULL,
            NULL,
            NULL,
            NULL);
        if (hThread == NULL)
        {
            // AfxMessageBox("MyCreateRemoteThread() : NtCreateThreadEx() 调用失败！错误代码: [%d]/n", GetLastError());
            return FALSE;
        }
    }
    else                    // 2000, XP, Server2003  
    {
        hThread = CreateRemoteThread(hProcess,
            NULL,
            0,
            pThreadProc,
            pRemoteBuf,
            0,
            NULL);
        if (hThread == NULL)
        {
            // AfxMessageBox("MyCreateRemoteThread() : CreateRemoteThread() 调用失败！错误代码: [%d]/n", GetLastError());
            return FALSE;
        }
    }
    if (WAIT_FAILED == WaitForSingleObject(hThread, INFINITE))
    {
        // AfxMessageBox("MyCreateRemoteThread() : WaitForSingleObject() 调用失败！错误代码: [%d]/n", GetLastError());
        return FALSE;
    }
    return hThread;
}
//--------------------------------------------------------------------- 
//在目标进程中创建线程并注入dll
BOOL   CreateRemoteThreadLoadDll(LPCWSTR   lpwLibFile, DWORD64   dwProcessId)
{
    BOOL   bRet = FALSE;
    HANDLE   hProcess = NULL, hThread = NULL;
    LPVOID pszLibRemoteFile = NULL;
    SIZE_T dwWritten = 0;
    __try
    {
        //1.打开进程，同时申请权限，这里申请了PROCESS_ALL_ACCESS
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, (DWORD)dwProcessId);
        if (hProcess == NULL)
            __leave;

        int   cch = 1 + lstrlenW(lpwLibFile);
        int   cb = cch * sizeof(WCHAR);
        // AfxMessageBox("cb:%d\n", cb);
        // AfxMessageBox("cb1:%d\n", sizeof(lpwLibFile));
        //2.申请足够的空间，以便把我们的dll写入目标进程中这个空间里
        pszLibRemoteFile = VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);

        if (pszLibRemoteFile == NULL)
            __leave;
        //3.正式把我们的dll写入上面申请的空间
        BOOL   bw = WriteProcessMemory(hProcess, pszLibRemoteFile, (PVOID)lpwLibFile, cb, &dwWritten);
        if (dwWritten != cb)
        {
            // AfxMessageBox("write error!\n");
        }
        if (!bw)
            __leave;
        //4.获得关键函数LoadLibraryW地址
        PTHREAD_START_ROUTINE   pfnThreadRnt = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"),
#ifdef UNICODE
            "LoadLibraryW"
#else
            "LoadLibrary"
#endif
        );
        if (pfnThreadRnt == NULL)
            __leave;
        //5.创建线程并把LoadLibraryW作为线程起始函数，传给LoadLibraryW的参数是我们的dll
        hThread = MyCreateRemoteThread(hProcess, pfnThreadRnt, pszLibRemoteFile);
        if (hThread == NULL)
            __leave;
        //6.等待上一步完成
        WaitForSingleObject(hThread, INFINITE);

        bRet = TRUE;
    }
    __finally
    {
        if (pszLibRemoteFile != NULL)
            VirtualFreeEx(hProcess, pszLibRemoteFile, 0, MEM_RELEASE);

        if (hThread != NULL)
            CloseHandle(hThread);

        if (hProcess != NULL)
            CloseHandle(hProcess);
    }

    return   bRet;
}
//--------------------------------------------------------------------- 
//卸载dll
BOOL   CreateRemoteThreadUnloadDll(LPCWSTR   lpwLibFile, DWORD64   dwProcessId)
{
    BOOL   bRet = FALSE;
    HANDLE   hProcess = NULL, hThread = NULL;
    HANDLE   hSnapshot = NULL;

    __try
    {
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, (DWORD)dwProcessId);
        if (hSnapshot == NULL)
            __leave;
        MODULEENTRY32W   me = { sizeof(MODULEENTRY32W) };
        BOOL   bFound = FALSE;
        BOOL   bMoreMods = Module32FirstW(hSnapshot, &me);
        for (; bMoreMods; bMoreMods = Module32NextW(hSnapshot, &me))
        {
            bFound = (lstrcmpiW(me.szModule, lpwLibFile) == 0) ||
                (lstrcmpiW(me.szExePath, lpwLibFile) == 0);
            if (bFound)
                break;
        }

        if (!bFound)
            __leave;

        hProcess = OpenProcess(
            PROCESS_CREATE_THREAD |
            PROCESS_VM_OPERATION,
            FALSE, (DWORD)dwProcessId);

        if (hProcess == NULL)
            __leave;

        PTHREAD_START_ROUTINE   pfnThreadRnt = (PTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "FreeLibrary ");
        if (pfnThreadRnt == NULL)
            __leave;

        //hThread   =   CreateRemoteThread(hProcess,   NULL,   0,pfnThreadRnt,   me.modBaseAddr,   0,   NULL); 
        hThread = MyCreateRemoteThread(hProcess, pfnThreadRnt, me.modBaseAddr);

        if (hThread == NULL)
            __leave;

        WaitForSingleObject(hThread, INFINITE);

        bRet = TRUE;
    }
    __finally
    {
        if (hSnapshot != NULL)
            CloseHandle(hSnapshot);

        if (hThread != NULL)
            CloseHandle(hThread);

        if (hProcess != NULL)
            CloseHandle(hProcess);
    }

    return   bRet;
}
//--------------------------------------------------------------------- 
//提升程序权限
BOOL   EnableDebugPrivilege()
{
    HANDLE   hToken;
    BOOL   fOk = FALSE;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        TOKEN_PRIVILEGES   tp;
        tp.PrivilegeCount = 1;
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {};
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL));
        else
            fOk = TRUE;
        CloseHandle(hToken);
    }
    return   fOk;
}
//--------------------------------------------------------------------- 
//为当前进程增加指定的特权 
int   AddPrivilege(LPCSTR   *Name)
{
    HANDLE   hToken;
    TOKEN_PRIVILEGES   tp;
    LUID   Luid;

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken))
    {
#ifdef   _DEBUG 
        // AfxMessageBox("OpenProcessToken   error.\n ");
#endif 
        return   1;
    }

    if (!LookupPrivilegeValue(NULL, *Name, &Luid))
    {
#ifdef   _DEBUG 
        // AfxMessageBox("LookupPrivilegeValue   error.\n ");
#endif 
        return   1;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tp.Privileges[0].Luid = Luid;

    if (!AdjustTokenPrivileges(hToken,
        0,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        NULL,
        NULL))
    {
#ifdef   _DEBUG 
        // AfxMessageBox("AdjustTokenPrivileges   error.\n ");
#endif 
        return   1;
    }

    return   0;
}
//--------------------------------------------------------------------- 
//注入函数，调用该函数即可
int injectDll()
{
    BOOL result = FALSE;
    //提升权限
    result = EnableDebugPrivilege();
    if (result != TRUE)
    {
        // AfxMessageBox("add privilege failed!\n");
        return -1;
    }
    PROCESSENTRY32   pe32 = { 0 };
    //打开进程快照
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == (HANDLE)-1)
    {
        return   -1;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    //获取目标进程的ID
    if (Process32First(hProcessSnap, &pe32))   //获取第一个进程 
    {
        do {
            char te[MAX_PATH];
            strcpy(te, pe32.szExeFile);
            if (strcmp(te, INJECT_PROCESS_NAME) == 0)
            {
                dwRemoteProcessId = pe32.th32ProcessID;
                // AfxMessageBox("%I64d\n", (UINT)dwRemoteProcessId);
                break;
            }
        } while (Process32Next(hProcessSnap, &pe32));//获取下一个进程 
    }
    else
    {
        return   -1;
    }

    WCHAR   wsz[MAX_PATH];
    swprintf(wsz, L"%S ", "./hook.dll"); //dll地址

    LPCWSTR   p = wsz;
    //在目标进程中创建线程并注入dll
    if (CreateRemoteThreadLoadDll(p, dwRemoteProcessId))
        return 1;
    return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return true;
    case WM_COMMAND:
        switch (wParam)
        {
        case(IDOK):
            DestroyWindow(hDlg);
            PostQuitMessage(0);
            return TRUE;
        }
    }
    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

extern"C" int __declspec(dllexport) ShowDialog()
{
    static TCHAR szAppName[] = TEXT("EXIT");
    HWND         hwnd;
    MSG          msg;
    WNDCLASS     wndclass;
    HINSTANCE hInstance;
    HWND hDialog;

    hInstance = LoadLibrary("./hook.dll");

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = DLGWINDOWEXTRA;    // Note!
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, szAppName);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
    hwnd = CreateWindow(szAppName, TEXT("exit"),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);
    hDialog = CreateDialog(hInstance, MAKEINTRESOURCE(200), hwnd, DlgProc);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (hDialog == 0 || !IsDialogMessage(hDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}

LRESULT WINAPI MsgProcWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//对外开放的接口
int ExitWindow(HWND hWndParent)
{
    parent_hWnd = GetForegroundWindow();
    parent_hInst = GetModuleHandle(NULL);

    WNDCLASSEX wcex0 = { sizeof(WNDCLASSEX),CS_HREDRAW | CS_VREDRAW,MsgProcWindow,0,0,parent_hInst,LoadIcon(NULL,IDI_INFORMATION),LoadCursor(NULL, IDC_ARROW),(HBRUSH)GetStockObject(BLACK_BRUSH),NULL,_T("Window0"),NULL };

    RegisterClassEx(&wcex0);
    child_hWnd = NULL;
    child_hWnd = CreateWindow(_T("EXIT"), _T("EXIT"), WS_OVERLAPPEDWINDOW | WS_CHILD, 50, 50, 200, 200, parent_hWnd, NULL, wcex0.hInstance, NULL);
    if (!child_hWnd)
    {
        MessageBox(NULL, _T("子窗口创建失败"), _T("Error"), MB_OK);
    }

    MSG msg;
    ShowWindow(child_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(child_hWnd);

    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnregisterClass(_T("EXIT"), wcex0.hInstance);
    return 1;
}
