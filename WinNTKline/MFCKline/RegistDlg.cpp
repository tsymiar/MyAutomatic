// Regist.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCKline.h"
#include "RegistDlg.h"


// CRegist 对话框

IMPLEMENT_DYNAMIC(CRegistDlg, CDialogEx)

CRegistDlg::CRegistDlg(char* ip, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_BROWSER, pParent)
	, m_browser()
{
	if (ip == nullptr)
		return;
	memcpy(&this->ip, ip, sizeof(ip) * sizeof(char*) + 1);
}

CRegistDlg::~CRegistDlg()
{
}

void CRegistDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRegistDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CRegistDlg::OnInitDialog()
{
	MessageBox("即将弹出的对话框，请点击“是”。");
	Init();	
	return \
		CDialogEx::OnInitDialog();
}


void CRegistDlg::Init()
{
	CRect cet;
	CString url;
	int reg_port = 443;
	COleVariant noAg;
	GetClientRect(&cet);
	if (!m_browser.Create(NULL, NULL, WS_VISIBLE, \
		cet, this, ID_BROSR))return;
	if (this->ip == nullptr)return;
	url.Format("https://%s:%d/Regist", this->ip, reg_port);
	m_browser.Navigate(url, &noAg, &noAg, &noAg, &noAg);
}

