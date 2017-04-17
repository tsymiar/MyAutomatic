#pragma once



class IMlogDlg : public CDialogEx
{
	DECLARE_DYNAMIC(IMlogDlg)

public:
	virtual ~IMlogDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GOIM };
#endif

protected:

	DECLARE_MESSAGE_MAP()
};
