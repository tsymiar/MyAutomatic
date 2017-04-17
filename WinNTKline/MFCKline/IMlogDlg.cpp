//

#include "stdafx.h"
#include "MFCKline.h"
#include "IMlogDlg.h"
#include "afxdialogex.h"



IMPLEMENT_DYNAMIC(IMlogDlg, CDialogEx)

IMlogDlg::IMlogDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_GOIM, pParent)
{

}

IMlogDlg::~IMlogDlg()
{
}

void IMlogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(IMlogDlg, CDialogEx)
END_MESSAGE_MAP()

