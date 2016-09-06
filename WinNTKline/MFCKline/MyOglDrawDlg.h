#pragma once
//#include	<afxstat_.h>
#include	"OGL/OGLKview.h"
#include	"Def/MacroDef.h"
#include	"Chart/DepthChart.h"
#include	"font/FontNehe.h"
#include	"ogl/GlModel.h"
#include	"Index/CommSet.h"
#include	"own/CNMenu.h"
#include	"MFCKline.h"
#include	"NET/tcpip.h"
#include	"afxdialogex.h"

using namespace freetype;
// MyOglDrawDlg 对话框

class MyOglDrawDlg : public CDialog
{
	DECLARE_DYNAMIC(MyOglDrawDlg)

public:
	MyOglDrawDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~MyOglDrawDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OGLWIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(tagMSG* pMsg);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
#ifdef _UNICODE
	afx_msg void OnTimer(UINT_PTR nIDEvent);
#else
	afx_msg void OnTimer(UINT nIDEvent);
#endif // !_UNICODE

	DECLARE_MESSAGE_MAP()
private:
	int W;
	int H;
	float dX;
	float dY;
	float oldX = 0;
	float oldY = 0;
	int failmsg = 0;
	char* notify;
private:
	NOTIFYICONDATA myNotify(HWND O_hWnd);
	void SetCurrentPosition(float winx, float winy);
	void _stdcall DrawFunc(HDC m_hDC);
	bool GetMarkDatatoDraw();
	void PostNcDestroy();
	void SetCtrl();
public:
	HDC		m_hDC;
	OGLKview  Ogl;
	HICON m_hIcon;
	HACCEL m_hAcc;
	CString title;
	CTabCtrl m_tab;
	CToolBar m_tool;
	GlModel model;
	std::vector<char*> markdata;
	OGLKview::Market trademarket;
	DepthChart *depth= DepthChart::getDepth();
	std::vector<struct OGLKview::Market > market;
public:
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnSize(UINT nType,int cx,int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSysCommand(UINT nID, LPARAM lparam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd,UINT nHitTest,UINT message);
	afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
	afx_msg LRESULT GetOglCmd(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT ShowMsgOnly(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnTaskShow(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT CALLBACK WindowProc(
		_In_ HWND   hwnd,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
		);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void FloatDrift(char* text);
	afx_msg void OnClose();
	afx_msg void ToQuit();
	afx_msg void SetBkg();
	afx_msg void Set_5_Deg();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPriv();
};
