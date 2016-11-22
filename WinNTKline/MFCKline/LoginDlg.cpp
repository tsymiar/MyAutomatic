// LoginDlg.cpp : 实现文件
//
#include "MFCKline.h"
#include "LoginDlg.h"
#include "RegistDlg.h"

//应加到具体源文件中
#include "web\myweb.h"
#include "myweb.nsmap"
#include "security\MD5.h"

struct soap soap;
char* ip = "192.168.1.17";
CString STrslt;
char* st[8];
char cmd[64];
char m_port[16];
// CLoginDlg 对话框

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOGIN, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void Ip2Str(CIPAddressCtrl &m_ipAddr, CString &strIP)
{
	unsigned  char  *pIP;
	DWORD  dwIP;
	m_ipAddr.GetAddress(dwIP);
	pIP = (unsigned  char*)&dwIP;
	strIP.Format("%u.%u.%u.%u", *(pIP + 3), *(pIP + 2), *(pIP + 1), *pIP);
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PORT, m_logport);
	DDX_Control(pDX, IDC_REGI, m_regist);
	DDX_Control(pDX, IDC_IP, m_combo);
	DDX_Control(pDX, IDC_IPADDR, m_ipCtrl);
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_LOGIN, &CLoginDlg::OnBnClickedLogin)
	ON_BN_CLICKED(IDCANCEL, &CLoginDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_DFT80, &CLoginDlg::OnBnClickedDft80)
	ON_STN_CLICKED(IDC_REGI, &CLoginDlg::OnStnClickedRegi)
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_IP, &CLoginDlg::OnCbnSelchangeIp)
END_MESSAGE_MAP()

BOOL CLoginDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	soap_init(&soap);
	SetCombo();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CLoginDlg::OnBnClickedLogin()
{
	char dig[32],out[32],addr[256];
	::GetDlgItemText(this->m_hWnd, IDC_ACNT, m_acnt, 32);
	::GetDlgItemText(this->m_hWnd, IDC_PSW, m_pswd, 32);
	::GetDlgItemText(this->m_hWnd, IDC_PORT, m_port, 32);
	if (!strcmp(m_port, ""))
	{
		MessageBox("请输入端口号！");
		return;
	}
	md5_str(m_pswd, dig);
	hex_to_str((unsigned char*)dig, out);
	sprintf_s(cmd, "Login@acc=%s&psw=%s", m_acnt, get_Hash(out, 16, out));
	sprintf(addr, "http://%s:%s/myweb.cgi", ip, m_port);
	//CloseHandle((HANDLE)_beginthreadex(NULL, 0, \
	//	(_beginthreadex_proc_type)soap_call_api__encrypt(&soap, addr, "", cmd, st)\
	//	, NULL, CREATE_SUSPENDED, 0));
	int err = soap_call_api__encrypt(&soap, addr, "", cmd, st);
	switch (err)
	{
	case 0:
		if (st[0] == NULL)
			STrslt.Format("%s", st[0]);
		break;
	case -1:
		STrslt.Format("服务崩溃");
		break;
	case 28:
	case 403:
		STrslt.Format("网络错误");
		break;
	default:
		STrslt.Format("%s", st[0]);
		break;
	}
	::PostMessage(this->m_hWnd, WM_MSG_BOX, 0,
		(LPARAM)netmsg.AllocBuffer(STrslt));
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
	CRegistDlg m_crgist(ip);
	m_crgist.DoModal();
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

void CLoginDlg::SetCombo()
{
	char* IPs[8];
	CString dftip;
	dftip.Format("默认(%s)", ip);
	m_combo.InsertString(0, _T(dftip));
	GetiniIPs(IPs);
	m_combo.InsertString(1, _T("自定义1"));
	m_combo.InsertString(2, _T("自定义2"));
	m_combo.SetCurSel(0);  // 默认选择第一项  
}

void CLoginDlg::GetiniIPs(char* IPs[])
{
#define N 8
	int index;
	CString detail[N] = { _T("") };
	GetPrivateProfileString(_T("default"), _T("detail"), "", detail[0].GetBuffer(MAX_PATH), MAX_PATH, "cfg//ips.ini");
	index = GetPrivateProfileInt("default", "index", 10, "cfg//ips.ini") - 10086;	//读入整型值
	CWnd* p1 = GetDlgItem(IDC_ACNT);
	p1->SetWindowText(detail[0]);
}

void CLoginDlg::OnCbnSelchangeIp()
{
	CString sip, temp;
	int index = m_combo.GetCurSel();
	m_combo.GetLBText(index, sip);
	switch (index)
	{
	case 0:
		AfxMessageBox(sip);
		break;
	case 1:
		AfxMessageBox(sip);
		break;
	case 2:
		AfxMessageBox(sip);
		break;
	}
}
