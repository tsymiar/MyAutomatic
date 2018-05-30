//

#pragma once

class CtestUtilsDlg : public CDialogEx
{

public:
	CtestUtilsDlg(CWnd* pParent = NULL);
	~CtestUtilsDlg();
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
	afx_msg void OnBnClickedCtp();
	afx_msg void OnBnClickedSimbtn();
	afx_msg void OnBnClickedImser();
private:
	CIPAddressCtrl m_ipAddr;
	CEdit m_Port;
	char a_IP[16];
	CString a_Port;
	inline void checkLeak(void* ptr)
	{
		if (ptr != nullptr)
			delete ptr;
	};
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
