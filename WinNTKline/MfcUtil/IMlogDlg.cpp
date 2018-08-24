// IMlogDlg.cpp : 瀹炵幇鏂囦欢
//

#include "stdafx.h"
#include "MFCKline.h"
#include "IMlogDlg.h"
#include "afxdialogex.h"

// IMlogDlg 瀵硅瘽妗?

IMPLEMENT_DYNAMIC(IMlogDlg, CDialogEx)

IMlogDlg::IMlogDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_IMMODAL, pParent) {}

IMlogDlg::IMlogDlg(int(*func)(char *, char *))
{
    setUsrPsw = func;
}

IMlogDlg::~IMlogDlg()
{
}

void IMlogDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ACNT, m_editUsr);
    DDX_Control(pDX, IDC_PSW, m_editPsw);
}

BEGIN_MESSAGE_MAP(IMlogDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &IMlogDlg::OnBnClickedOk)
END_MESSAGE_MAP()



void IMlogDlg::PostNcDestroy()
{
}

BOOL IMlogDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_editUsr.SetLimitText(20);
    m_editPsw.SetLimitText(20);
    return 0;
}

void IMlogDlg::OnBnClickedOk()
{
    GetDlgItem(IDC_ACNT)->GetWindowText(m_strAcnt);
    GetDlgItem(IDC_PSW)->GetWindowText(m_strPsw);
    if (lstrlen(m_strAcnt) < 3)
    {
        AfxMessageBox("请输入3~20个字符。");
        return;
    }
    else
        if (!checkPswValid((LPSTR)(LPCSTR)m_strPsw)) {
            AfxMessageBox("密码必须是数字、字母和特殊字符的集合！");
            return;
        }
    if (setUsrPsw((LPSTR)(LPCSTR)m_strAcnt, (LPSTR)(LPCSTR)m_strPsw))
        //GetDlgItem(IDC_LISTFRND)->ShowWindow(SW_SHOW);
        CDialogEx::OnOK();
}
