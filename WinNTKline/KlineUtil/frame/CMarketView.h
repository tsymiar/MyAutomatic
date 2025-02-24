#pragma once

// CMarketView 窗体视图

class CMarketView : public CFormView
{
	DECLARE_DYNCREATE(CMarketView)

protected:
	CMarketView();           // 动态创建所使用的受保护的构造函数
	virtual ~CMarketView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FORMVIEW };
#else
#define IDD_FORMVIEW "CMarketView"
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnInitialUpdate();
	DECLARE_MESSAGE_MAP()
	BOOL CreateChildrenWindow();
};
