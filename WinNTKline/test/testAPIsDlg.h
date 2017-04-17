//

#pragma once

class CtestAPIsDlg : public CDialogEx
{

public:
	CtestAPIsDlg(CWnd* pParent = NULL);
	~CtestAPIsDlg();
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TEST_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedToim();
	afx_msg void OnBnClickedRegist();
	afx_msg void OnBnClickedTestlog();
	afx_msg void OnBnClickedKline();
};
