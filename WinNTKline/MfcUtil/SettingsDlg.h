#pragma once
#include "afxwin.h"

class SettingsDlg : public CDialogEx
{
    DECLARE_DYNAMIC(SettingsDlg)

public:
    SettingsDlg(CWnd* pParent = NULL);
    SettingsDlg(void(*func)(char*));
    virtual ~SettingsDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MARKDLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

    DECLARE_MESSAGE_MAP()
private:
    int callback = 0;
    bool checkMark = true;
    CEdit m_Marks;
    CString cs_Mark;
    void(*setPswCallback)(char* psw);
public:
    int SetTitle(CString title);
    CString& GetMark(CString& text, CString& title);
    afx_msg void OnBnClickedOk();
    bool SetifCheck(bool check);
};
