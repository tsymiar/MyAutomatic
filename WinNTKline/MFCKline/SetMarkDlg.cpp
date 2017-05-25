#include "stdafx.h"
#include "afxdialogex.h"
#include "Resource.h"
#include "SetMarkDlg.h"

IMPLEMENT_DYNAMIC(SetMarkDlg, CDialogEx)

SetMarkDlg::SetMarkDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MARKDLG, pParent)
{}

SetMarkDlg::~SetMarkDlg() {}

void SetMarkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MARKS, m_Marks);
}

BEGIN_MESSAGE_MAP(SetMarkDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &SetMarkDlg::OnBnClickedOk)
END_MESSAGE_MAP()

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
	CDialogEx::OnOK();
}
