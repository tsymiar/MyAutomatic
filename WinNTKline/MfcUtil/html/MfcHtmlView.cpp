// MyHtmlView.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCKline.h"
#include "MfcHtmlView.h"

// CHTMLDlg

IMPLEMENT_DYNCREATE(CMfcHTML, CHtmlView)

CMfcHTML::CMfcHTML(){}

CMfcHTML::~CMfcHTML(){}

void CMfcHTML::DoDataExchange(CDataExchange* pDX)
{
	CHtmlView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMfcHTML, CHtmlView)
END_MESSAGE_MAP()


// CHTMLDlg 诊断

#ifdef _DEBUG
void CMfcHTML::AssertValid() const
{
	CHtmlView::AssertValid();
}

void CMfcHTML::Dump(CDumpContext& dc) const
{
	CHtmlView::Dump(dc);
}
#endif //_DEBUG
/*
BOOL CMfcHTML::CreateControlSite(COleControlContainer * pContainer, COleControlSite ** ppSite, UINT, REFCLSID)
{
	*ppSite = new CDocHostSite(pContainer, this);
	return (*ppSite) ? TRUE : FALSE;
}

HRESULT CDocHostSite::XDocHostUIHandler::ShowContextMenu(DWORD dwID,
	POINT * ppt,
	IUnknown * pcmdtReserved,
	IDispatch * pdispReserved)
{
	METHOD_PROLOGUE(CDocHostSite, DocHostUIHandler);
	return pThis->m_pView->OnShowContextMenu(dwID, ppt, pcmdtReserved, pdispReserved);
}
*/
BOOL CMfcHTML::CreateFromDialog(CWnd* pDlgWnd)
{
	CRect rc;
	pDlgWnd->GetClientRect(&rc);

	int nID = pDlgWnd->GetDlgCtrlID();
	LPCTSTR lpClassName = AfxRegisterWndClass(NULL);
	if (NULL != pDlgWnd->GetSafeHwnd())
		return Create(lpClassName, _T(""), WS_CHILD | WS_VISIBLE,
			rc, pDlgWnd, nID, NULL);
	else
	{
		ASSERT(0);
		return FALSE;
	}
}

int CMfcHTML::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// TODO: Add your message handler code here and/or call default 

	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);

	// Do not call it. 
	//return CHtmlView::OnMouseActivate(pDesktopWnd, nHitTest, message); 
}

void CMfcHTML::OnNavigateError(LPCTSTR lpszURL, LPCTSTR lpszFrame,
	DWORD dwError, BOOL *pbCancel)
{
	if (200 != dwError && 0 == _tcscmp(this->GetLocationURL(), lpszURL))
	{
		// Navigate to local html file. 
		GetParent()->PostMessage(WM_MSG_NAVIURL, 0, 0);
		return;
	}
	return CHtmlView::OnNavigateError(lpszURL, lpszFrame, dwError, pbCancel);
}
