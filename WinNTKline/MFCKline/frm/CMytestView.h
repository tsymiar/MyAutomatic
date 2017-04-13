#pragma once

// CMytestView 窗体视图

class CMytestView : public CFormView
{
	DECLARE_DYNCREATE(CMytestView)

protected:
	CMytestView();           // 动态创建所使用的受保护的构造函数
	virtual ~CMytestView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FORMVIEW };
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
};


