// CMytestView.cpp : 实现文件
//

#include "../stdafx.h"
#include "../MarketClient.h"
#include "CMytestView.h"


// CMytestView

IMPLEMENT_DYNCREATE(CMytestView, CFormView)

CMytestView::CMytestView()
	: CFormView(IDD_FORMVIEW)
{

}

CMytestView::~CMytestView()
{
}

void CMytestView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMytestView, CFormView)
END_MESSAGE_MAP()


// CMytestView 诊断

#ifdef _DEBUG
void CMytestView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CMytestView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


void CMytestView::OnInitialUpdate()
{
	
}

/*
BOOL ::CreateChildrenWindow()
{
	//CtestDlg test;
	//LONG styleValue = ::GetWindowLong(test.m_hWnd, GWL_STYLE);
	//styleValue &= ~WS_POPUP;
	//::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CHILD | WS_VISIBLE | WS_EX_LAYERED);
	//CRect ect;
	//::GetWindowRect(this->m_hWnd, ect);
	//::SetWindowPos(test.m_hWnd, NULL, ect.left, ect.top, ect.Width(), ect.Height(), SWP_NOZORDER);
	//test.DoModal();
	if (!m_mySplitter.CreateStatic(this, 1, 2))//切割客户区为1行2列
		return FALSE;
	if (!m_mySplitter.CreateView(0, 0, RUNTIME_CLASS(CMytestView), CSize(0, 0), NULL)) //第一行第一列展示的View视图
		return FALSE;
	if (!m_mySplitter.CreateView(0, 1, RUNTIME_CLASS(CMytestView), CSize(0, 0), NULL)) //第一行第二列展示的View视图
		return FALSE;
	CRect ect;
	GetClientRect(&ect);//获取客户区的大小
	int width = ect.Width() + 200;//设置显示对话框的区域大小
	m_mySplitter.SetColumnInfo(0, width, 0);
	m_mySplitter.RecalcLayout();
	return TRUE;
}
*/
