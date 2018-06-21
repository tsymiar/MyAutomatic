#pragma once
//#include	<afxstat_.h>
#include	"MFCKline.h"
#include	"afxdialogex.h"
#include	"MYGL/OGLKview.h"
#include	"netwk/CTPclient.h"
#include	"deep/DepthMarket.h"
#include	"font/FontNehe.h"
#include	"Idx/CommSet.h"
#include	"dos/DOSCout.h"
#include	"mygl/GlModel.h"
#include	"netwk/Simulation.h"
#include	"own/Screen-Sharing.h"

using namespace freetype;
// MyOglDrawDlg 对话框

class MyOglDrawDlg : public CDialog
{
	DECLARE_DYNAMIC(MyOglDrawDlg)

public:
	MyOglDrawDlg() {};
	MyOglDrawDlg(const char* sIP, CWnd* pParent = NULL);   // 标准构造函数
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
	float offsetX;
	float offsetY;
	float lastX = 0;
	float lastY = 0;
	HDC		m_hDC;
	HICON m_hIcon;
	HACCEL m_hAcc;
	CRITICAL_SECTION mutex;
private:
	NOTIFYICONDATA GetNotiIcon(HWND O_hWnd);
	void SetCurrentPosition(float winx, float winy);
	void _stdcall DrawFunc(HDC m_hDC);
	void PostNcDestroy();
	void SetCtrl();
public:
	char* ctpIP = NULL;
	int kPos = 0;
	OGLKview  Ogl;
	CString title;
	GlModel model;
	CTabCtrl m_tab;
	CToolBar m_tool;
	DOSCout Dos;
	DepthMarket object;
	DepthMarket *depth= object.getDepth();
public:
	// 重载函数
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSysCommand(UINT nID, LPARAM lparam);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnSize(UINT nType,int cx,int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pnt);
	afx_msg BOOL OnSetCursor(CWnd* pWnd,UINT nHitTest,UINT message);
	afx_msg LRESULT CALLBACK WindowProc(
		_In_ HWND   hwnd,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
		);
	afx_msg void OnClose();
	// 自定义消息函数
	afx_msg LRESULT GetOglCmd(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT ShowMsgOnly(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnTaskShow(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT SetDlgTitle(WPARAM wparam, LPARAM lparam);
	afx_msg void FloatDrift(char* text);
	afx_msg void SetDlgBkg();
	afx_msg void SetDeepDeg(); 
	void CallShellScript(CString Path, CString fbat, CString param);
	char* GetFirstData();
	auto GetPrivMem();
	int GetMarkDatatoDraw(int sign = 0, int len = 0);
	afx_msg void OnSetBkg();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnScrn2Bmp();
};
