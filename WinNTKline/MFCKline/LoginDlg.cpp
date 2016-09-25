// LoginDlg.cpp : 实现文件
//
#include "MFCKline.h"
//应加到具体源文件中
#include "web/web.h"
#include "web/web.nsmap"

#include "LoginDlg.h"
#include "stdafx.h"
#include "afxdialogex.h"

// CLoginDlg 对话框

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOGINDLG, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PORT, m_logport);
	DDX_Control(pDX, IDC_ACNT, m_account);
	DDX_Control(pDX, IDC_PSW, m_password);
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_LOGIN, &CLoginDlg::OnBnClickedLogin)
	ON_BN_CLICKED(IDCANCEL, &CLoginDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_DFT80, &CLoginDlg::OnBnClickedDft80)
END_MESSAGE_MAP()


// CLoginDlg 消息处理程序


void CLoginDlg::OnBnClickedLogin()
{
	std::string s;
	ns2__encrypt("dvf",s);
}


void CLoginDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}


void CLoginDlg::OnBnClickedDft80()
{
	m_logport.SetWindowText("80");
}
