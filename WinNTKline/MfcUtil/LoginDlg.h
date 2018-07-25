#pragma once
#include "stdafx.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "afxmt.h"
#include <afxdialogex.h>
#include "MyOglDrawDlg.h"
#include "IMhideWndDlg.h"
// CLoginDlg 对话框

class CLoginDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLoginDlg)

public:
	CLoginDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CLoginDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	HICON m_hIcon;
	DECLARE_MESSAGE_MAP()
private:
	struct IPS {
		char name[32];
		char user[16];
		char addr[16];
		char port[5];
	};
#define MAX_INI 8
	struct IPS st_ini[MAX_INI];
	CHAR m_IP[16];
	char m_acnt[32];
	char m_pswd[32];
	int cnt_flg = 0;
	CFont m_font;
	CEdit m_logport;
	CEdit m_editPsw;
	CStatic m_regist;
	CComboBox m_combo;
	MyOglDrawDlg m_Gl;
	MyOglDrawDlg* m_ComGL = NULL;
	CIMhideWndDlg* m_IMwnd = NULL;
	CIMhideWndDlg* m_ComIM = NULL;
	CIPAddressCtrl m_ipCtrl;
	void SetCombox();
	void SetUserofini0();
	void getNsetIPs();
public:
	afx_msg void OnCbnSelchangeCom();
	afx_msg void OnBnClickedCombox();
	afx_msg void OnBnClickedDft80();
	afx_msg void OnStnClickedRegi();
	afx_msg void OnBnClickedLogin();
	afx_msg void OnBnClickedCancel();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	INT_PTR testRegist(char* m_IP);
	BOOL testLogin(void* log);
};
