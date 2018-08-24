#include "stdafx.h"
#include "afxdialogex.h"
#include "Resource.h"
#include "SettingsDlg.h"
#include "IM/IMClient.h"

IMPLEMENT_DYNAMIC(SettingsDlg, CDialogEx)

SettingsDlg::SettingsDlg(CWnd * pParent)
    : CDialogEx(IDD_MARKDLG, pParent)
{
}

SettingsDlg::SettingsDlg(void(*func)(char*))
{
    setPswCallback = func;
    callback = 1;
}

SettingsDlg::~SettingsDlg() {}

void SettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MARKS, m_Marks);
}

BEGIN_MESSAGE_MAP(SettingsDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &SettingsDlg::OnBnClickedOk)
END_MESSAGE_MAP()

int SettingsDlg::SetTitle(CString title)
{
    if (!title.IsEmpty())
        ::SetWindowText(this->m_hWnd, title);
    else
        return 1;
    return 0;
}

CString & SettingsDlg::GetMark(CString& text, CString& title)
{
    if (!title.IsEmpty())
        ::SetWindowText(this->m_hWnd, title);
    if (m_Marks.m_hWnd != 0)
        m_Marks.SetWindowText(text);
    return cs_Mark;
}

void SettingsDlg::OnBnClickedOk()
{
    m_Marks.GetWindowText(cs_Mark);
    if (!checkPswValid((LPSTR)(LPCSTR)cs_Mark)) {
        AfxMessageBox("密码必须是数字、字母和特殊字符的集合！");
        return;
    }
    if (callback)
        setPswCallback((LPTSTR)(LPCTSTR)cs_Mark);
    CDialogEx::OnOK();
}

