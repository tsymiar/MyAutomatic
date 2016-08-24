
// MFCKlineDlg.h : 头文件
//

#pragma once
#include "MyOglDrawDlg.h"
#include "Wpf/WpfHost.h"

// CMFCKlineDlg 对话框
class CMFCKlineDlg : public CDialogEx
{
// 构造
public:
	CMFCKlineDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CMFCKlineDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCKLINE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOgl();
	afx_msg void OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnItemMsg(WPARAM wParam, LPARAM lParam);
public:
	CListCtrl m_list;
	MyOglDrawDlg m_Mod;
	OGLKview m_Ogl;
private:
	void SetList();
	void InitMap();
	void SetBottom();
	bool OpenGL(int item);
	void OpenWpf();
	void OpenQt();
	int OpenQtexe();
private:
	HWND m_hBottom;
};
