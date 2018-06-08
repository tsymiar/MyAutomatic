#pragma once

// CMfcKView 窗体视图

class CMfcKView : public CFormView
{
	DECLARE_DYNCREATE(CMfcKView)

protected:
	CMfcKView();           // 动态创建所使用的受保护的构造函数
	virtual ~CMfcKView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCKLINE };
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif
	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
};


