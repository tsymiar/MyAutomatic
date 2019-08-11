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
    this->visable = 0;
}

void IMlogDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ACNT, m_editUsr);
    DDX_Control(pDX, IDC_PSW, m_editPsw);
}

BEGIN_MESSAGE_MAP(IMlogDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &IMlogDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &IMlogDlg::OnBnClickedCancel)
END_MESSAGE_MAP()



void IMlogDlg::PostNcDestroy()
{
    this->visable = 0;
}

BOOL IMlogDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_editUsr.SetLimitText(20);
    m_editPsw.SetLimitText(20);
    return 0;
}

BOOL IMlogDlg::Create(UINT Tmp, CWnd * Wnd)
{
    this->visable = 1;
    return CDialog::Create(Tmp, Wnd);
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
        if (!(m_strPsw.Compare("AAA") || m_strPsw.Compare("tesT123@"))) {
            if (!checkPswValid((LPSTR)(LPCSTR)m_strPsw)) {
                AfxMessageBox("密码必须是数字、字母和特殊字符的集合！");
                return;
            }
        }
    if (setUsrPsw((LPSTR)(LPCSTR)m_strAcnt, (LPSTR)(LPCSTR)m_strPsw))
        CDialogEx::OnOK();
}

int IMlogDlg::getVisable()
{
    return this->visable;
}

void IMlogDlg::setVisable(int visable)
{
    this->visable = visable;
}

void IMlogDlg::OnBnClickedCancel()
{
    this->visable = 0;
    CDialogEx::OnCancel();
}

char* IMlogDlg::getUsername() 
{
    return (LPSTR)(LPCSTR)(m_strAcnt);
}
