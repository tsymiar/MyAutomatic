#pragma once
#include "afxwin.h"

class SettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SettingsDlg)

public:
	SettingsDlg(CWnd* pParent = NULL);
	virtual ~SettingsDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MARKDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
	CEdit m_Marks;
	CString cs_Mark;
public:
	int SetTitle(CString title);
	CString& GetMark(CString& text, CString& title);
	afx_msg void OnBnClickedOk();
};
