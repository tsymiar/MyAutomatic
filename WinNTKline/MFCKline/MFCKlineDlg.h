
// MFCKlineDlg.h : 头文件
//

#pragma once
//#include <QtCore/QTextCodec>
//#include <QtWidgets/QApplication>
#include "MyOglDrawDlg.h"
#include "LoginDlg.h"
#include "Wpf/WpfHost.h"
#include <crtdbg.h>

#undef QT_FASTCALL
//“__fastcall”与“/clr”选项不兼容: 转换为“__stdcall”

// CMFCKlineDlg 对话框
class CMFCKlineDlg : public CDialogEx
{
// 构造
public:
	CMFCKlineDlg(bool ok = false, CWnd* pParent = NULL);	// 自定义构造函数
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
	bool isok = false;
	bool tolog = true;
	int item = 65535;
	HWND m_hBottom;
	CLoginDlg logdlg;
public:
	afx_msg void OnBnClickedOK();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnDropFiles(HDROP hDropInfo);
};
