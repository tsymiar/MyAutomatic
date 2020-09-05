// Regist.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCKline.h"
#include "RegistDlg.h"

const char hintext[] = "鼠标双击该处可退出本对话框";
// CRegist 对话框

IMPLEMENT_DYNAMIC(CRegistDlg, CDialogEx)

CRegistDlg::CRegistDlg(char* IP, CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_BROWSER, pParent)
    //, m_browser()
{
    if (IP == nullptr)
        return;
    memcpy(&this->m_IP, IP, sizeof(IP) * sizeof(char*) + 1);
}

CRegistDlg::~CRegistDlg()
{
}

CRegistDlg::CRegistDlg()
{
}

void CRegistDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRegistDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_STN_DBLCLK(ID_STATIC, &CRegistDlg::OnStnDblclickCursor)
END_MESSAGE_MAP()

int ShowBox(HWND hWndParent)
{
    HMODULE g_hmodu = LoadLibrary(_T("HOOK/hook.dll"));
    typedef int(*ADDROC) ();
    ADDROC ExitWindow = (ADDROC)GetProcAddress(g_hmodu, "ExitWindow");

    if (!ExitWindow)
    {
        MessageBox(hWndParent, _T("获取函数地址失败。"), _T("Error"), MB_OK);
        return -1;
    }
    ExitWindow();
    return 0;
}

BOOL CRegistDlg::OnInitDialog()
{
    InitBrs();
    SetTipLatent();
    // ShowBox(this->m_hWnd);
    return \
        CDialog::OnInitDialog();
}

void CRegistDlg::InitBrs()
{
    CRect rect;
    CString url;
    int reg_port = 443;
    COleVariant noAg;
    GetClientRect(&rect);
    if (this->m_IP[0] == '\0')return;
    int gauge = rect.right / 3;
    m_hint.Create(hintext, WS_VISIBLE | SS_NOTIFY, \
    { gauge, rect.bottom - 40, gauge + 170, rect.bottom - 20}, this, ID_STATIC);
    url.Format("https://%s:%d/MyAutomatic", this->m_IP, reg_port);
    /*
    //CWebBrowser2
    if (!m_browser.Create(NULL, NULL, WS_VISIBLE, \
    rect, this, ID_BROSR))return;
    m_browser.Navigate(url, &noAg, &noAg, &noAg, &noAg);
    */

    m_HTMLPage = new CMfcHtmlView;
    m_HTMLPage->CreateFromDialog(this);
    m_HTMLPage->Navigate2(url);

    //SetCefWeb(rect, url);
}

void CRegistDlg::SetTipLatent()
{
    m_Mytip.Create(this);
    m_Mytip.AddTool(GetDlgItem(ID_STATIC), hintext);
    m_Mytip.SetDelayTime(300);
    memset(&m_lf, 0, sizeof(LOGFONT));
    m_lf.lfHeight = 14; strcpy(m_lf.lfFaceName, "宋体");
    m_font.CreateFontIndirect(&m_lf);
    m_Mytip.SetFont(&m_font);
    m_Mytip.SetTipTextColor(RGB(0, 0, 255));
    m_Mytip.SetTipBkColor(RGB(230, 145, 56));
    m_Mytip.Activate(TRUE);
}

void CRegistDlg::SetCefWeb(RECT rect, CString& url)
{
    CefRefPtr<CEFWebKit>client(new CEFWebKit());
    m_cWebClient = client;
    CefMainArgs main_args(AfxGetApp()->m_hInstance);
    CefRefPtr<ClientAppRenderer> m_cefApp = new ClientAppRenderer();
    /*
    // Parse command-line arguments.
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromString(::GetCommandLineW());
    // The command-line flag won't be specified for the browser process.
    if (!command_line->HasSwitch("type"))
    {
    }
    else
    {
        const std::string& processType = command_line->GetSwitchValue("type");
        if (processType == "renderer")
        {
            m_cefApp = new ClientAppRenderer();
        }
    }
    */
    if (CefExecuteProcess(main_args, m_cefApp, NULL) >= 0) return;
    CefSettings cSettings;
    cSettings.no_sandbox = true;
    cSettings.single_process = false;
    cSettings.multi_threaded_message_loop = true;
    //CefSettingsTraits::init(&cSettings);
    CefInitialize(main_args, cSettings, m_cefApp.get(), NULL);
    CefWindowInfo window_info;
    window_info.SetAsChild(GetSafeHwnd(), rect);
    CefString sCS;
    CStringW sW((LPCTSTR)url, url.GetLength());
    sCS.FromString((const wchar_t*)sW, sW.GetLength(), true);
    CefBrowserSettings b_settings;
    CefBrowserHost::CreateBrowser(window_info, static_cast<CefRefPtr<CefClient>>(m_cWebClient).get(),
        sCS, b_settings, NULL);
}

BOOL CRegistDlg::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    // TODO: Add your specialized code here and/or call the base class 
    if (WM_MSG_NAVIURL == message)
    {
        m_HTMLPage->Navigate(_T("e:\\index.html"));
        return TRUE;
    }
    return CDialog::OnWndMsg(message, wParam, lParam, pResult);
}

void CRegistDlg::OnStnDblclickCursor()
{
    CDialog::OnOK();
}

BOOL CRegistDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_MOUSEMOVE)
    {
        SetCursor(LoadCursor(NULL, IDC_HAND));
        m_Mytip.RelayEvent(pMsg);
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}
