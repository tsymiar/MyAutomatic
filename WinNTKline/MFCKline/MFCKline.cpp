
// MFCKline.cpp : 定义应用程序的类行为。
//
#include "MFCKline.h"
#include "MFCKlineDlg.h"


// CMFCKlineApp

BEGIN_MESSAGE_MAP(CMFCKlineApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMFCKlineApp 构造

CMFCKlineApp::CMFCKlineApp()
{
    // 支持重新启动管理器
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CMFCKlineApp 对象

CMFCKlineApp theApp;


// CMFCKlineApp 初始化

BOOL CMFCKlineApp::InitInstance()
{
    // 如果一个运行在 Windows XP 上的应用程序清单指定要
    // 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
    //则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // 将它设置为包括所有要在应用程序中使用的
    // 公共控件类。
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();


    AfxEnableControlContainer();

    // 创建 shell 管理器，以防对话框包含
    // 任何 shell 树视图控件或 shell 列表视图控件。
    CShellManager *pShellManager = new CShellManager;

    // 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // 标准初始化
    // 如果未使用这些功能并希望减小
    // 最终可执行文件的大小，则应移除下列
    // 不需要的特定初始化例程
    // 更改用于存储设置的注册表项
    SetRegistryKey(_T("WinNT-Kline"));

    // 单例进程模式  
    HANDLE hMutex = ::CreateMutex(NULL, TRUE, _T("CSSegment"));
    if (hMutex != NULL)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            AfxMessageBox(_T("失败：程序已经启动！"));
            CloseHandle(hMutex);
            return FALSE;
        }
    }
    // 根据内存申请次序人工设置断点
#ifdef _DEBUG
    //_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(3570);
#endif
    // 启动首个窗口
    CMFCKlineDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    //X86平台上当高级语言使用了结构化异常处理
    //编译器会在函数的首尾生成特定的代码片段(ntdll!__RtlUserThreadStart)
    //以构建运行时异常栈帧。
    //.......................  
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    if (nResponse == IDOK)
    {
        //  放置处理“确定”对话框代码
    }
    else if (nResponse == IDCANCEL)
    {
        //  放置处理“取消”对话框代码
    }
    else if (nResponse == -1)
    {
        TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
        TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
    }

    // 删除上面创建的 shell 管理器。
    if (pShellManager != NULL)
    {
        delete pShellManager;
    }

    // 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
    //  而不是启动应用程序的消息泵。
    return FALSE;
}

