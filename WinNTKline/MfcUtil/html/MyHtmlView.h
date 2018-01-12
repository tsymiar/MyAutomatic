#pragma once

#ifdef _WIN32_WCE
#error "Windows CE 不支持 CHtmlView。"
#endif 

// CMyHtmlView Html 视图
#include "Def\MacroDef.h"

class CHTMLDlg : public CHtmlView
{
	DECLARE_DYNCREATE(CHTMLDlg)
protected:
	CHTMLDlg();           // 动态创建所使用的受保护的构造函数
	virtual ~CHTMLDlg();
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	//virtual BOOL CreateControlSite(COleControlContainer * pContainer,	COleControlSite ** ppSite, UINT /*nID*/, REFCLSID /*clsid*/);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
public:
	BOOL CreateFromDialog(CWnd* pDlgWnd);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message); 
	afx_msg void OnNavigateError(LPCTSTR lpszURL, LPCTSTR lpszFrame, DWORD dwError, BOOL *pbCancel);
};

class CMyHtmlView : public CHTMLDlg {};