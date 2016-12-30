// LoginDlg.cpp : 实现文件
//
#include "MFCKline.h"
#include "LoginDlg.h"
#include "RegistDlg.h"

//应加到具体源文件中
#include "web\myweb.h"
#include "myweb.nsmap"
#include "security\MD5.h"
#define M 8
struct soap soap;
char ip[] = "192.168.1.3";
CString STrslt;
char cmd[64];
char m_port[16]; 
struct SOAPELE {
	struct soap soap;
	char addr[256];
	char cmd[64];
	char *rslt[8];
	int err;
} soapele;
// CLoginDlg 对话框

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOGIN, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

unsigned char* fix_str(unsigned char* str)
{
	for (int i = 0; i<(int)strlen((const char*)str); i++)
		switch (str[i])
		{
		case 0xcc:/*烫 未初始化*/
		case 0xCD:/*heapk(new)*/
		case 0xDD://已收回的堆(delete)
		case 0xFD://隔离（栅栏字节）字节 下标越界
		case 0xAB://Memory allocated by LocalAlloc()
		case 0xBAADF00D://	Memory allocated by LocalAlloc() with LMEM_FIXED,\
						//	but not yet written to.
		case 0xFEEEFEEE:/*  OS fill heap memory, which was marked for usage,\
							but wasn't allocated by HeapAlloc() or LocalAlloc()\
							Or that memory just has been freed by HeapFree().
						*/
			str[i] = '\0';
			break;
		default:break;
		}
	return str;
}

void fill_edit(CEdit& edit, CString& tmp)
{
	CString sTemp;
	edit.GetWindowText(sTemp);
	if (sTemp.IsEmpty())
		sTemp += (tmp + "\r\n");
	else
		sTemp += ("\r\n" + tmp);
	edit.SetWindowText(sTemp);
	edit.LineScroll(edit.GetLineCount(), 0);
}

void CLoginDlg::fill_edit(CEdit& edit, const char tmp[], int hexlen)
{
	CString sTmp;
	char* temp = new char[2048];
	if (edit.m_hWnd == NULL)return;
	edit.GetWindowText(sTmp);
	if (sTmp.GetLength() >= 0x7f0)
		sTmp = "";
	if (sTmp.IsEmpty())
	{
		sprintf(temp, "%s", tmp);
	}
	else
		if (hexlen > 0)
		{
			unsigned char* uTmp = (unsigned char*)tmp;
			for (int i = 0; i < hexlen; i++)
			{
				if (i == 0)
					sprintf(temp, "%02X ", uTmp[0]);
				if (!(i%16)) 
					sprintf(temp, "%s %02X\n", sTmp, uTmp[i]);
				else
					sprintf(temp, "%s %02X", sTmp, uTmp[i]);
			}
		}
		else
		{
			sprintf(temp, "%s\r\n%s", sTmp, tmp);
		}
	edit.SetWindowText(temp);
	delete[] temp;
	edit.LineScroll(edit.GetLineCount(), 0);
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

unsigned int _stdcall call_soap_thrd(void* lp)
{
	SOAPELE* ele = (SOAPELE*)lp;
	ele->err = soap_call_api__encrypt(&ele->soap, ele->addr, "", ele->cmd, (char**)&ele->rslt);
	return ele->err;
}

void CLoginDlg::OnBnClickedLogin()
{
	char dig[32],out[32],addr[256];	
	memset(&soapele, 0, sizeof(SOAPELE));
	::GetDlgItemText(this->m_hWnd, IDC_ACNT, m_acnt, 32);
	::GetDlgItemText(this->m_hWnd, IDC_PSW, m_pswd, 32);
	::GetDlgItemText(this->m_hWnd, IDC_PORT, m_port, 32);
	if (!strcmp(m_port, ""))
	{
		AfxMessageBox("请输入端口号！");
		return;
	}
	md5_str(m_pswd, dig);
	hex_to_str((unsigned char*)dig, out);
	sprintf_s(cmd, "Login@acc=%s&psw=%s", m_acnt, get_Hash(out, 16, out));
	sprintf(addr, "http://%s:%s/myweb.cgi", ip, m_port);
	soapele.soap = soap;
	soapele.rslt[0] = (char *)malloc(sizeof(char) * M * 8);
	for (int i = 1; i < M; i++)
		soapele.rslt[i] = soapele.rslt[i - 1] + 8;
	soapele.rslt[0][0] = NULL;
	memcpy(soapele.cmd, cmd, sizeof(cmd));
	memcpy(soapele.addr, addr, sizeof(addr));
	//CloseHandle((HANDLE)_beginthreadex(NULL, 0, \
	//	(_beginthreadex_proc_type)soap_call_api__encrypt(&soap, addr, "", cmd, st)\
	//	, NULL, CREATE_SUSPENDED, 0));
	CloseHandle((HANDLE)_beginthreadex(NULL, 0, \
		(_beginthreadex_proc_type)&call_soap_thrd\
		, (void *)&soapele, 0, NULL));
	//int err = soap_call_api__encrypt(&soap, addr, "", cmd, rslt);
	switch (soapele.err)
	{
	case 0:
		if (*soapele.rslt[0] == '\0')
			STrslt.Format("返回错误");
		break;
	case -1:
		STrslt.Format("服务崩溃");
		break;
	case 28:
	case 403:
		STrslt.Format("网络错误");
		break;
	default:
		STrslt.Format("%s", *soapele.rslt[0]);
		break;
	}
	AfxMessageBox(STrslt);
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
	dftip.Format("默认");
	m_combo.InsertString(0, _T(dftip));
	GetiniIPs(IPs);
	m_combo.InsertString(1, _T("ip1"));
	m_combo.InsertString(2, _T("ip2"));
	m_combo.SetCurSel(0);  // 默认选择第一项  
}

void CLoginDlg::GetiniIPs(char* IPs[])
{
	int index;
#define N 8
	CString detail[N] = { _T("") };
	GetPrivateProfileString(_T("dft"), _T("usr"), "", detail[0].GetBuffer(MAX_PATH), MAX_PATH, "cfg//ips.ini");
	index = GetPrivateProfileInt("dft", "idx", 10, "cfg//ips.ini") - 10086;	//读入整型值
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
		m_ipCtrl.SetAddress(192,168,1,3);
		break;
	case 1:
		MessageBox(sip);
		break;
	case 2:
		MessageBox(sip);
		break;
	}
}
