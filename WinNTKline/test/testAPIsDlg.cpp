//

#include "stdafx.h"
#include "testAPIs.h"
#include "testAPIsDlg.h"
#include "afxdialogex.h"
#include <Resource.h>
#include <dos\DOSCout.h>
#include <soapH.h>
#include "LoginDlg.h"
#include "IMhideWndDlg.h"
#include "ctl\CWebBrowser2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct SOELE {
	struct soap soap;
	struct IMUSR imusr;
} soele;

CLoginDlg logon;
IMPLEMENT_DYNCREATE(CWebBrowser2, CWnd)


CtestAPIsDlg::CtestAPIsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TEST_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CtestAPIsDlg::~CtestAPIsDlg()
{
}

void CtestAPIsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CtestAPIsDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_TOIM, &CtestAPIsDlg::OnBnClickedToim)
	ON_BN_CLICKED(IDC_REGIDST, &CtestAPIsDlg::OnBnClickedRegist)
	ON_BN_CLICKED(IDC_TESTLOG, &CtestAPIsDlg::OnBnClickedTestlog)
END_MESSAGE_MAP()


BOOL CtestAPIsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	sprintf_s(soele.imusr.addr, 32, "http://127.0.0.1:8080/myweb.cgi");
	sprintf_s(soele.imusr.usr, 11, "ioscatchme");
	sprintf_s(soele.imusr.psw, 17, "a6afbbcbf8be7668");

	return TRUE;
}


void CtestAPIsDlg::OnPaint()
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


HCURSOR CtestAPIsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CtestAPIsDlg::OnBnClickedToim()
{
	DOSCout dos;
	CIMhideWndDlg* m_pIM = new CIMhideWndDlg();
	dos.RedirectConsole();
	m_pIM->Create(IDD_IMHIDEWND);
	m_pIM->ShowWindow(SW_SHOWNORMAL);
}

void CtestAPIsDlg::OnBnClickedRegist()
{
	logon.testRegist(soele.imusr.addr);
}

void CtestAPIsDlg::OnBnClickedTestlog()
{
	//for (int i = 0; i < 100; i++)
		logon.testLogin(&soele);
}
