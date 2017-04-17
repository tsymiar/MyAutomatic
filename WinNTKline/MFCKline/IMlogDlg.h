#pragma once


// IMlogDlg 对话框

class IMlogDlg : public CDialogEx
{
	DECLARE_DYNAMIC(IMlogDlg)

public:
	IMlogDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~IMlogDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GOIM };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
