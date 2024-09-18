#pragma once
#include "IM/Client.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "IMlogDlg.h"

// CIMhideWndDlg 对话框

class CIMhideWndDlg : public CDialogEx {
    DECLARE_DYNAMIC(CIMhideWndDlg)

public:
    CIMhideWndDlg(StSock* sock = nullptr, CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CIMhideWndDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_IMHIDEWND };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    //{{AFX_MSG(CIMhideWndDlg)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
    //修正移动时窗口的大小
    void FixMoving(UINT fwSide, LPRECT pRect);
    //从收缩状态显示窗口
    void DoShow();
    //从显示状态收缩窗口
    void DoHide();
    //重载函数,只是为了方便调用
    BOOL SetWindowPos(const CWnd* pWndInsertAfter,
        LPCRECT pCRect, UINT nFlags = SWP_SHOWWINDOW);
    afx_msg void OnSysCommand(UINT nID, LPARAM lparam);
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    BOOL m_isSizeChanged;   //窗口大小是否改变了 
    BOOL m_isSetTimer;      //是否设置了检测鼠标的Timer

    INT  m_oldWndHeight;    //旧的窗口宽度
    INT  m_taskBarHeight;   //任务栏高度
    INT  m_edgeHeight;      //边缘高度
    INT  m_edgeWidth;       //边缘宽度

    INT  m_hideMode;        //隐藏模式
    BOOL m_hsFinished;      //隐藏或显示过程是否完成
    BOOL m_hiding;          //该参数只有在!m_hsFinished才有效
                            //真:正在隐藏,假:正在显示
    StSock m_imSocks;
    IMlogDlg* m_logDlg = NULL;
    
    int logflg = 0;
    int m_count = 0;
    HICON m_hIcon;
    HBITMAP hBitmap = NULL;
    CComboBox m_combo;
    CStatic m_timeStatus;
    CButton m_AddBtn;
public:
    CStatic m_username;
    CListCtrl m_frndList;
    void setFriendList();
    void setWidgetHide();
    afx_msg void OnBnClickedExit();
    afx_msg void OnCbnSelchangeComm();
    afx_msg void OnNMDblclkListfrnd(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnPaint();
    afx_msg void OnBnClickedSeeknew();
    afx_msg BOOL DestroyWindow();
};
