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

void CMfcHTML::ShieldCurrPage(CComPtr<IHTMLDocument2> &parentDoc, BSTR m_bstrScript)
{
	CComPtr<IHTMLWindow2>  spIhtmlwindow2;
	parentDoc->get_parentWindow(reinterpret_cast<IHTMLWindow2**>(&spIhtmlwindow2));
	if (spIhtmlwindow2 != NULL)
	{
		CString strLanguage("JavaScript");
		BSTR bstrLanguage = strLanguage.AllocSysString();
		long lTime = 1 * 1000;
		long lTimeID = 0;
		VARIANT varLanguage;
		varLanguage.vt = VT_BSTR;
		varLanguage.bstrVal = bstrLanguage;
		VARIANT pRet;
		//把window.onerror函数插入入页面
		spIhtmlwindow2->execScript(m_bstrScript, bstrLanguage, &pRet);
		::SysFreeString(bstrLanguage);
	}
}

void CMfcHTML::WalkAllChildPages(CComPtr<IHTMLDocument2> &parentDoc, BSTR m_bstrScript)
{
	CComPtr<IHTMLFramesCollection2> spFramesCol;
	HRESULT hr = parentDoc->get_frames(&spFramesCol);
	if (SUCCEEDED(hr) && spFramesCol != NULL)
	{
		long lSize = 0;
		hr = spFramesCol->get_length(&lSize);
		if (SUCCEEDED(hr))
		{
			for (int i = 0; i<lSize; i++)
			{
				VARIANT frameRequested;
				VARIANT frameOut;
				frameRequested.vt = VT_UI4;
				frameRequested.lVal = i;
				hr = spFramesCol->item(&frameRequested, &frameOut);
				if (SUCCEEDED(hr) && frameOut.pdispVal != NULL)
				{
					CComPtr<IHTMLWindow2> spChildWindow;

					hr = frameOut.pdispVal->QueryInterface(IID_IHTMLWindow2, reinterpret_cast<void**>(&spChildWindow));
					if (SUCCEEDED(hr) && spChildWindow != NULL)
					{
						CComPtr<IHTMLDocument2> spChildDocument;
						hr = spChildWindow->get_document(reinterpret_cast<IHTMLDocument2**>(&spChildDocument));
						if (SUCCEEDED(hr) && spChildDocument != NULL)
						{
							ShieldCurrPage(spChildDocument, m_bstrScript);
							WalkAllChildPages(spChildDocument, m_bstrScript);
						}
					}
					frameOut.pdispVal->Release();
				}
			}
		}
	}
}

void CMfcHTML::OnNavigateComplete2(LPCTSTR strURL)
{
	BSTR m_bstrScript = CMfcHTML::strJavaScriptCode.AllocSysString();
	CComPtr<IDispatch>   spDisp = GetHtmlDocument();
	if (spDisp != NULL)
	{
		CComPtr<IHTMLDocument2> parentDoc;
		spDisp->QueryInterface(IID_IHTMLDocument2, reinterpret_cast<void**>(&parentDoc));
		if (parentDoc != NULL)
		{
			ShieldCurrPage(parentDoc, m_bstrScript);
			WalkAllChildPages(parentDoc, m_bstrScript);
		}
	}
	SysFreeString(m_bstrScript);
}
