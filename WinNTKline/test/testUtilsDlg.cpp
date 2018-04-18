//

#include "stdafx.h"
#include "testUtils.h"
#include "testUtilsDlg.h"
#include "afxdialogex.h"
#include <Resource.h>
#include <dos\DOSCout.h>
#include <soapH.h>
#include "MyOglDrawDlg.h"
#include "LoginDlg.h"
#include "IMhideWndDlg.h"
#include "html\CWebBrowser2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define LOOP_TIME 1

struct SOELEM {
	struct soap soap;
	struct IMUSR imusr;
} soelem;

CLoginDlg logon; 
CTPclient* m_ctp = nullptr;
MyOglDrawDlg *ogl = nullptr;
CIMhideWndDlg *m_pIM = nullptr;

IMPLEMENT_DYNCREATE(CWebBrowser2, CWnd)

CtestUtilsDlg::CtestUtilsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TEST_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CtestUtilsDlg::~CtestUtilsDlg()
{
}

void CtestUtilsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPCTRL, m_ipAddr);
	DDX_Control(pDX, IDC_MPORT, m_Port);
}

BEGIN_MESSAGE_MAP(CtestUtilsDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_TOIM, &CtestUtilsDlg::OnBnClickedToim)
	ON_BN_CLICKED(IDC_REGIDST, &CtestUtilsDlg::OnBnClickedRegist)
	ON_BN_CLICKED(IDC_TESTLOG, &CtestUtilsDlg::OnBnClickedTestlog)
	ON_BN_CLICKED(IDC_KLINE, &CtestUtilsDlg::OnBnClickedKline)
	ON_BN_CLICKED(IDC_CTP, &CtestUtilsDlg::OnBnClickedCtp)
	ON_BN_CLICKED(IDC_SIMBTN, &CtestUtilsDlg::OnBnClickedSimbtn)
	ON_BN_CLICKED(IDC_IMSER, &CtestUtilsDlg::OnBnClickedImser)
	ON_BN_CLICKED(IDOK, &CtestUtilsDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CtestUtilsDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


BOOL CtestUtilsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	m_ipAddr.GetWindowText(s_IP, 16);
	if (s_IP[0] == '\0' || s_IP[0] == '\x30')
	{
		m_ipAddr.SetAddress(192, 168, 1, 3);
		sprintf_s(s_IP, 16, "192.168.1.3");
	}

	m_Port.GetWindowText(s_Port);
	if (s_Port.IsEmpty())
	{
		s_Port = "8080";
		m_Port.SetWindowText(s_Port);
	}

	return TRUE;
}


void CtestUtilsDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CtestUtilsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CtestUtilsDlg::OnBnClickedToim()
{
	m_pIM = new CIMhideWndDlg((IMUSR*)"127.0.0.1");
	m_pIM->Create(IDD_IMHIDEWND);
	m_pIM->ShowWindow(SW_SHOWNORMAL);
}

void CtestUtilsDlg::OnBnClickedRegist()
{
	m_ipAddr.GetWindowText(s_IP, 16);
	logon.testRegist(s_IP);
}

void CtestUtilsDlg::OnBnClickedTestlog()
{
	m_Port.GetWindowText(s_Port); 
	m_ipAddr.GetWindowText(s_IP, 16);
	sprintf_s(soelem.imusr.addr, 64, "http://%s:%d/myweb.cgi", s_IP, atoi(s_Port));
	sprintf_s(soelem.imusr.usr, 11, "ioscatchme");
	sprintf_s(soelem.imusr.psw, 17, "a6afbbcbf8be7668");

	for (int i = 0; i < LOOP_TIME; i++)
	{
		logon.testLogin(&soelem);
		Sleep(10);
	}
}


void CtestUtilsDlg::OnBnClickedKline()
{
	ogl = new MyOglDrawDlg();
	ogl->Create(IDD_OGLIMG);
	ogl->ShowWindow(SW_SHOWNORMAL);
}


void CtestUtilsDlg::OnBnClickedCtp()
{
	m_ctp = new CTPclient();
	CloseHandle((HANDLE)_beginthreadex(NULL, 0, m_ctp->TradeMarket, (void*)this, 0, NULL));
}


void CtestUtilsDlg::OnBnClickedSimbtn()
{
	if (AllocConsole())
	{
		freopen("CONOUT$", "w", stderr);
		Simulation();
	}
}

void CtestUtilsDlg::OnBnClickedImser()
{
	STARTUPINFO sInfo;
	PROCESS_INFORMATION pInfo;
	ZeroMemory(&pInfo, sizeof(pInfo));
	ZeroMemory(&sInfo, sizeof(sInfo));
	sInfo.cb = sizeof(sInfo);
	sInfo.dwFlags = STARTF_USESHOWWINDOW;
	sInfo.wShowWindow = SW_SHOWNORMAL;
	::CreateProcess(_T("..\\Debug\\IM(Win32).exe"), _T("1"), NULL, NULL, false, 0, NULL, NULL, &sInfo, &pInfo);
}


void CtestUtilsDlg::OnBnClickedOk()
{
	checkLeak(ogl);
	checkLeak(m_ctp);
	checkLeak(m_pIM);
	CDialog::OnOK();
}

void CtestUtilsDlg::OnBnClickedCancel()
{
	OnBnClickedOk();
}