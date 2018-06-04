#pragma once

#ifdef _WIN32_WCE
#error "Windows CE 不支持 CHtmlView。"
#endif 

// CMfcHtmlView Html 视图
#include "Def\MacroDef.h"

class CMfcHTML : public CHtmlView
{
	DECLARE_DYNCREATE(CMfcHTML)
protected:
	CMfcHTML();           // 动态创建所使用的受保护的构造函数
	virtual ~CMfcHTML();
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	//virtual BOOL CreateControlSite(COleControlContainer * pContainer,	COleControlSite ** ppSite, UINT /*nID*/, REFCLSID /*clsid*/);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
private:
	CString strJavaScriptCode = "function fnOnError(msg,url,lineno){alert('script error:\\n\\nURL:'+url"
		"+'\\n\\nMSG:'+msg +'\\n\\nLine:'+lineno+'\\n\\nframes:' + window.frames.length);return true;}window.onerror=fnOnError;";
	void WalkAllChildPages(CComPtr<IHTMLDocument2> &parentDoc, BSTR m_bstrScript);
	void ShieldCurrPage(CComPtr<IHTMLDocument2> &parentDoc, BSTR m_bstrScript);
public:
	BOOL CreateFromDialog(CWnd* pDlgWnd);
	virtual void OnNavigateComplete2(LPCTSTR strURL);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message); 
	afx_msg void OnNavigateError(LPCTSTR lpszURL, LPCTSTR lpszFrame, DWORD dwError, BOOL *pbCancel);
};

class CMfcHtmlView : public CMfcHTML {};