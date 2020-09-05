#pragma once
#include "afxwin.h"

class SetMarkDlg : public CDialogEx
{
    DECLARE_DYNAMIC(SetMarkDlg)

public:
    SetMarkDlg(CWnd* pParent = NULL);
    SetMarkDlg(void(*func)(char*));
    virtual ~SetMarkDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MARKDLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

    DECLARE_MESSAGE_MAP()
private:
    int callback = 0;
    bool checkMark = false;
    CEdit m_Marks;
    CString cs_Mark;
    void(*settingCallback)(char* psw);
public:
    int SetTitle(CString title);
    CString& GetMark(CString& text, CString& title);
    afx_msg void OnBnClickedOk();
    bool SetifCheck(bool check);
    void SetCallback(void(*func)(char*));
    int GetCallTimes();
};
