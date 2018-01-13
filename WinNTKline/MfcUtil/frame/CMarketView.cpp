// CMarketView.cpp : 实现文件
//

#include "../stdafx.h"
#include "../MarketClient.h"
#include "CMarketView.h"


// CMarketView

IMPLEMENT_DYNCREATE(CMarketView, CFormView)

CMarketView::CMarketView()
	: CFormView(IDD_FORMVIEW)
{

}

CMarketView::~CMarketView()
{
}

void CMarketView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMarketView, CFormView)
END_MESSAGE_MAP()


// CMarketView 诊断

#ifdef _DEBUG
void CMarketView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CMarketView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


void CMarketView::OnInitialUpdate()
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
	if (!m_mySplitter.CreateView(0, 0, RUNTIME_CLASS(CMarketView), CSize(0, 0), NULL)) //View视图
		return FALSE;
	if (!m_mySplitter.CreateView(0, 1, RUNTIME_CLASS(CMarketView), CSize(0, 0), NULL))
		return FALSE;
	CRect ect;
	GetClientRect(&ect);//获取客户区的大小
	int width = ect.Width() + 200;//设置显示对话框的区域大小
	m_mySplitter.SetColumnInfo(0, width, 0);
	m_mySplitter.RecalcLayout();
	return TRUE;
}
*/
