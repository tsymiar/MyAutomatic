#pragma once

#include "ctl/CWebBrowser2.h"
#include "afxdialogex.h"

// CRegist 对话框

class CRegistDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegistDlg)

public:
	CRegistDlg(char* ip, CWnd* pParent = NULL);   // 标准构造函数
	virtual BOOL OnInitDialog();
	virtual ~CRegistDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BROWSER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CWebBrowser2 m_browser;
	char ip[32];
private:
	void Init();
};
