#include "stdafx.h"
#include "afxdialogex.h"
#include "Resource.h"
#include "SetMarkDlg.h"
#include "IM/client.h"

IMPLEMENT_DYNAMIC(SetMarkDlg, CDialogEx)

SetMarkDlg::SetMarkDlg(CWnd * pParent)
    : CDialogEx(IDD_MARKDLG, pParent)
{
    settingCallback = NULL;
}

SetMarkDlg::SetMarkDlg(void(*func)(char*))
{
    settingCallback = func;
    callback = 1;
}

SetMarkDlg::~SetMarkDlg() {}

void SetMarkDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MARKS, m_Marks);
}

BEGIN_MESSAGE_MAP(SetMarkDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &SetMarkDlg::OnBnClickedOk)
END_MESSAGE_MAP()

int SetMarkDlg::SetTitle(CString title)
{
    if (!title.IsEmpty())
        ::SetWindowText(this->m_hWnd, title);
    else
        return 1;
    return 0;
}

CString & SetMarkDlg::GetMark(CString& text, CString& title)
{
    if (!title.IsEmpty())
        ::SetWindowText(this->m_hWnd, title);
    if (m_Marks.m_hWnd != 0)
        m_Marks.SetWindowText(text);
    return cs_Mark;
}

void SetMarkDlg::OnBnClickedOk()
{
    m_Marks.GetWindowText(cs_Mark);
    if (!!!checkPswValid((LPSTR)(LPCSTR)cs_Mark) && checkMark) {
        AfxMessageBox("输入请包含数字、字母和特殊字符！");
        return;
    }
    if (callback)
        settingCallback((LPTSTR)(LPCTSTR)cs_Mark);
    CDialogEx::OnOK();
}

bool SetMarkDlg::SetifCheck(bool check)
{
    if(check)
        m_Marks.SetPasswordChar('*');
    return this->checkMark = check;
}

void SetMarkDlg::SetCallback(void(*func)(char*))
{
    settingCallback = func;
    callback++;
}

int SetMarkDlg::GetCallTimes() 
{
    return callback;
}