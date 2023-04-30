// LoginDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "MFCKline.h"
#include "LoginDlg.h"
#include "RegistDlg.h"
#include "netwk/CTPclient.h"
#include "netwk/Simulation.h"

//应加到具体源文件中
#include "service/myweb.h"
#include "myweb.nsmap"
#include "security\MD5.h"
#include "security/encoding.h"

#include "opencv/CvimgMat.h"

namespace logDlg
{
#define M 8
    char dftip[] = "127.0.0.1";
    CString STrslt;
    char form[80];
    char m_Port[16];

    struct SOAPELEM {
        struct soap soap;
        StSock sock;
        struct api__ArrayOfEmp2 rslt;
        StMsgContent user;
        char msg[512];
        char** __rslt;
        int error;
    } elem;
}

using namespace logDlg;

// CLoginDlg 对话框

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_LOGINDLG, pParent)
{
}

CLoginDlg::~CLoginDlg()
{
    if (m_ComGL != NULL)
    {
        delete m_ComGL;
        m_ComGL = NULL;
    }
    if (m_ComIM != NULL)
    {
        delete m_ComIM;
        m_ComIM = NULL;
    }
    if (m_IMwnd != NULL)
    {
        delete m_IMwnd;
        m_IMwnd = NULL;
    }
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PORT, m_logport);
    DDX_Control(pDX, IDC_REGI, m_regist);
    DDX_Control(pDX, IDC_COMBOX, m_combo);
    DDX_Control(pDX, IDC_IPADDR, m_ipCtrl);
    DDX_Control(pDX, IDC_PSW, m_editPsw);
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_LOGIN, &CLoginDlg::OnBnClickedLogin)
    ON_BN_CLICKED(IDCANCEL, &CLoginDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_DFT80, &CLoginDlg::OnBnClickedDft80)
    ON_STN_CLICKED(IDC_REGI, &CLoginDlg::OnStnClickedRegi)
    ON_CBN_SETFOCUS(IDC_COMBOX, &CLoginDlg::OnBnClickedCombox)
    ON_CBN_SELCHANGE(IDC_COMBOX, &CLoginDlg::OnCbnSelchangeCom)
END_MESSAGE_MAP()

BOOL CLoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);            // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    soap_init(&logDlg::elem.soap);
    SetCombox();
    m_editPsw.SetLimitText(16);
    m_ipCtrl.SetAddress(127, 0, 0, 1);
    m_editPsw.SetWindowText("tesT123$");
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

unsigned int _stdcall call_soap_thrd(void* lr)
{
    int sz_t = sizeof(SOAPELEM);
    SOAPELEM* elem = (SOAPELEM*)malloc(sz_t);
    memset(elem, 0, sz_t);
    elem->rslt = api__ArrayOfEmp2();
    elem->sock = StSock();
    memset(&elem->msg, 0, 32);
    memcpy(&elem->sock, lr, sizeof(StSock));
    if (elem->sock.addr[0] == NULL) {
        AfxMessageBox("获取请求链接错误！");
        return -1;
    }
    if (elem->soap.state == 0)
        soap_init(&elem->soap);
    elem->error = soap_call_api__trans(&elem->soap, elem->sock.addr, (char*)&(elem->user.uiCmdMsg), elem->sock.form, (char**)&elem->msg);
    CString msg;
    if (elem->error != 0)
    {
        soap_sprint_fault(&elem->soap, elem->msg, 512);
        msg.Format("%s\n%s", *soap_faultstring(&elem->soap), elem->msg);
    } else {
        char* status = new char[64];
        soap_call_api__get_server_status(&elem->soap, elem->sock.addr, "", "1000", status);
        msg.Format("登录成功\n%s\n%s", elem->msg, status);
        delete status;
    }
    AfxMessageBox(msg);
    return elem->error;
}

void CLoginDlg::OnBnClickedLogin()
{
    char hexmd5[32], out[32], addr[256];
    memset(&elem, 0, sizeof(SOAPELEM));
    ::GetDlgItemText(this->m_hWnd, IDC_ACNT, m_acnt, 32);
    ::GetDlgItemText(this->m_hWnd, IDC_PSW, m_pswd, 32);
    ::GetDlgItemText(this->m_hWnd, IDC_PORT, m_Port, 16);
    ::GetDlgItemText(this->m_hWnd, IDC_IPADDR, m_IP, 16);
    if (!strcmp(m_Port, ""))
    {
        AfxMessageBox("请输入端口号！");
        return;
    }
    md5_to_hex(m_pswd, hexmd5);
    hex_to_ascii((unsigned char*)hexmd5, out);
    memcpy(elem.user.password, get_Hash(out, 16, out), 16);
    sprintf_s(form, "Login@acc=%s&psw=%s", m_acnt, elem.user.password);
    if (m_IP[0] == '\0' || m_IP[0] == '\x30')
        sprintf_s(addr, 256, "http://%s:%s/myweb.cgi", dftip, m_Port);
    else
        sprintf_s(addr, 256, "http://%s:%s/myweb.cgi", m_IP, m_Port);
    elem.soap = logDlg::elem.soap;
    elem.__rslt = new char* [M];
    //elem.__rslt[0] = (char *)malloc(sizeof(char) * M * 8);
    memset(elem.__rslt, 0, 32);
    for (int i = 1; i < M; i++)
    {
        elem.__rslt[i] = new char[16];
        // elem.__rslt[i - 1] + 8;
    }
    memcpy(elem.sock.form, form, sizeof(form));
    memcpy(elem.sock.addr, addr, sizeof(addr));
    memcpy(elem.user.username, m_acnt, sizeof(m_acnt));
    memcpy(elem.sock.IP, m_IP, 16);
    //CloseHandle((HANDLE)_beginthreadex(NULL, 0, \
    //    (_beginthreadex_proc_type)&call_soap_thrd \
    //    , (void *)&elem, 0, NULL));
    elem.error = soap_call_api__login_by_key(&elem.soap, elem.sock.addr, "",
        (char*)elem.user.username, (char*)elem.user.password, elem.rslt);
    switch (elem.error)
    {
    case 0:
        if (elem.rslt.rslt.flag != 200)
            STrslt.Format("返回错误");
        else
        {
            m_Gl.Create(IDD_OGLIMG);
            m_Gl.ShowWindow(SW_SHOWNORMAL);
            m_IMwnd = new CIMhideWndDlg(&elem.sock);
            m_IMwnd->Create(IDD_IMHIDEWND);
            m_IMwnd->ShowWindow(SW_SHOWNORMAL);
            m_IMwnd->setWidgetHide();
            CDialog::OnOK();
            return;
        }
        break;
    case -1:
        STrslt.Format("服务崩溃");
        break;
    case 28:
    case 403:
        STrslt.Format("网络错误");
        break;
    default:
        STrslt.Format("%s:(%s)[%s]", *soap_faultcode(&elem.soap), elem.rslt.rslt.email, elem.rslt.rslt.tell);
        break;
    }
    AfxMessageBox(STrslt);
    delete[] elem.__rslt;
    //free(elem.__rslt);
}

void CLoginDlg::OnBnClickedCancel()
{
    CDialogEx::OnCancel();
}

void CLoginDlg::OnBnClickedDft80()
{
    m_logport.SetWindowText("8080");
}

void CLoginDlg::OnStnClickedRegi()
{
    if (AfxMessageBox("即将弹出的对话框，请点击“是”。", MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK)
    {
        m_ipCtrl.GetWindowText(m_IP, 16);
        if (m_IP[0] == '\0' || m_IP[0] == '\x30')
            memcpy(m_IP, dftip, 10);
        CRegistDlg m_crgist(m_IP);
        m_crgist.DoModal();
    }
}

HBRUSH CLoginDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_STATIC)
    {
        switch (pWnd->GetDlgCtrlID())
        {
        case IDC_REGI:
        {
            pDC->SetTextColor(RGB(0, 0, 255));
            pDC->SelectObject(&m_font);
            pDC->SetBkMode(TRANSPARENT);
        }
        default:
            break;
        }
    }
    return hbr;
}

void CLoginDlg::SetCombox()
{
    SetUserofini0();
    m_combo.InsertString(0, _T("默认"));
    m_combo.InsertString(1, _T("IM"));
    m_combo.InsertString(2, _T("CTP"));
    m_combo.InsertString(3, _T("Simulator"));
    m_combo.InsertString(4, _T("GLKline"));
    m_combo.InsertString(5, _T("CvimgMat"));
    m_combo.SetCurSel(0);  // 默认选择第一项
}

void CLoginDlg::SetUserofini0()
{
    CString user = { _T("") };
    // int cur = GetPrivateProfileInt((LPCTSTR)"0", (LPCTSTR)"", 10, (LPCTSTR)"cfg//ips.ini") - 10086; //读入整型值
    GetPrivateProfileString(_T("0"), _T("user"), (LPCTSTR)"", user.GetBuffer(MAX_PATH), MAX_PATH, (LPCTSTR)"cfg//ips.ini");
    CWnd* p1 = GetDlgItem(IDC_ACNT);
    p1->SetWindowText(user);
}

void CLoginDlg::getNsetIPs()
{
    if (cnt_flg > 0)
        return;
    CString file = (LPCTSTR)"cfg//ips.ini";
    char configStr[MAX_INI + 1];
    int secNum = GetPrivateProfileString(NULL, NULL, (LPCTSTR)"", configStr, MAX_INI, file);
    int cfgNum = 0;
    int offset = 0;
    char* curstr = configStr + offset;
    for (offset; offset < secNum;)
    {
        offset += strlen(curstr) + 1;
        curstr = configStr + offset;
        cfgNum++;
    }
    CString istr;
    if (cfgNum > MAX_INI)
        cfgNum = MAX_INI;
    for (int i = 0; i < cfgNum; i++)
    {
        istr.Format(_T("%d"), i);
        GetPrivateProfileString(istr, _T("name"), (LPCTSTR)"", st_ini[i].name, MAX_PATH, file);
        GetPrivateProfileString(istr, _T("user"), (LPCTSTR)"", st_ini[i].user, MAX_PATH, file);
        GetPrivateProfileString(istr, _T("addr"), (LPCTSTR)"", st_ini[i].addr, MAX_PATH, file);
        GetPrivateProfileString(istr, _T("port"), (LPCTSTR)"", st_ini[i].port, MAX_PATH, file);
        m_combo.InsertString(m_combo.GetCount() + i, st_ini[i].name);
    }
    cnt_flg = 1;
}

void CLoginDlg::OnCbnSelchangeCom()
{
    CString sip;
    CvimgMat image;
    CTPclient* m_ctp = NULL;
    int curNo = m_combo.GetCurSel();
    m_combo.GetLBText(curNo, sip);
    ::GetDlgItemText(this->m_hWnd, IDC_IPADDR, m_IP, 16);
    switch (curNo)
    {
    case 0:
        m_ipCtrl.SetAddress(127, 0, 0, 1);
        break;
    case 1:
    {
        memcpy(&elem.sock.IP, m_IP, 16);
        m_ComIM = new CIMhideWndDlg(&elem.sock);
        m_ComIM->Create(IDD_IMHIDEWND);
        m_ComIM->ShowWindow(SW_SHOWNORMAL);
    }
    break;
    case 2:
        CloseHandle((HANDLE)_beginthreadex(NULL, 0, m_ctp->TradeMarket, (void*)this, 0, NULL));
        delete m_ctp;
        m_ctp = NULL;
        break;
    case 3:
        if (AllocConsole())
        {
            if (freopen("CONOUT$", "w", stderr) != nullptr)
                Simulation();
        }
        break;
    case 4:
        m_ComGL = new MyOglDrawDlg();
        m_ComGL->Create(IDD_OGLIMG);
        m_ComGL->ShowWindow(SW_SHOWNORMAL);
        break;
    case 5:
        image.cvmatTest();
        break;
    case 6:
        m_ipCtrl.SetAddress(ntohl(inet_addr(st_ini[m_combo.GetCount() - curNo - 1].addr)));
        m_logport.SetWindowText(st_ini[m_combo.GetCount() - curNo - 1].port);
        break;
    default: break;
    }
}

void CLoginDlg::OnBnClickedCombox()
{
    getNsetIPs();
}

INT_PTR CLoginDlg::testRegist(char* m_IP)
{
    CRegistDlg m_crgist(m_IP);
    return m_crgist.DoModal();
}

BOOL CLoginDlg::testLogin(StSock& log)
{
    return CloseHandle((HANDLE)_beginthreadex(NULL, 0, \
        (_beginthreadex_proc_type)&call_soap_thrd\
        , &log, 0, NULL));
}
