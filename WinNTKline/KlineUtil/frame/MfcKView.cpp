// MfcKView.cpp : 实现文件
//

#include "../stdafx.h"
#include <shellapi.h>
#include <process.h>
#include "../MarketClient.h"
#include "MfcKView.h"


// CMfcKView

IMPLEMENT_DYNCREATE(CMfcKView, CFormView)

CMfcKView::CMfcKView()
	: CFormView(IDD_MFCKLINE)
{
#ifndef _WIN32_WCE
	EnableActiveAccessibility();
#endif

}

CMfcKView::~CMfcKView()
{
}

void CMfcKView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMfcKView, CFormView)
	ON_BN_CLICKED(IDC_BUTTON1, &CMfcKView::OnBnClickedButton1)
END_MESSAGE_MAP()

// CMfcKView 诊断

#ifdef _DEBUG
void CMfcKView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CMfcKView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG



void CMfcKView::OnInitialUpdate()
{
	// Initial
	/*
	MyOglDrawDlg *ogl = new MyOglDrawDlg();
	ogl->Create(IDD_OGLIMG);
	ogl->ShowWindow(SW_SHOWNORMAL);
	*/
}



void CMfcKView::OnBnClickedButton1()
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	HWND hChildWnd = NULL;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.wShowWindow = SW_SHOW;
	si.dwFlags = STARTF_USESHOWWINDOW;
	if (!CreateProcess(
		_T("MFCKline.exe")
		, NULL
		, NULL
		, FALSE
		, NULL
		, NULL
		, NULL
		, NULL
		, &si
		, &pi
	)) {
		AfxMessageBox(_T("运行错误"));
	}
}
