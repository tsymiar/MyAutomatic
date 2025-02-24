// CMarketView.cpp : 实现文件
//

#include "stdafx.h"
#include "CMarketView.h"
#include "../../TestUtils/testUtilsDlg.h"

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

BOOL CMarketView::CreateChildrenWindow()
{
	CtestUtilsDlg test;
	LONG styleValue = ::GetWindowLong(test.m_hWnd, GWL_STYLE);
	styleValue &= ~WS_POPUP;
	::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CHILD | WS_VISIBLE | WS_EX_LAYERED);
	CRect ect;
	::GetWindowRect(this->m_hWnd, ect);
	::SetWindowPos(test.m_hWnd, NULL, ect.left, ect.top, ect.Width(), ect.Height(), SWP_NOZORDER);
	test.DoModal();
	return TRUE;
}
