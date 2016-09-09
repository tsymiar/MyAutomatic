// MFCKlineDlg.cpp : 实现文件
//
#include "MFCKline.h"
#include "MFCKlineDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  WINDOW_TEXT_LENGTH MAX_PATH

struct ProcessWindow
{
	DWORD dwProcessId;
	HWND hwndWindow = NULL;
} window;

// CMFCKlineDlg 对话框

CMFCKlineDlg::CMFCKlineDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MFCKLINE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMFCKlineDlg::~CMFCKlineDlg()
{
}

void CMFCKlineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_list);
}

BEGIN_MESSAGE_MAP(CMFCKlineDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_MSG_LIST, OnItemMsg)
	ON_BN_CLICKED(IDC_OGL, &CMFCKlineDlg::OnBnClickedOgl)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, &CMFCKlineDlg::OnLvnItemchangedList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, &CMFCKlineDlg::OnNMDblclkList)
	ON_BN_CLICKED(IDOK, &CMFCKlineDlg::OnBnClickedOK)
END_MESSAGE_MAP()


// CMFCKlineDlg 消息处理程序

BOOL CMFCKlineDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	InitMap();
	SetList();
	SetBottom();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCKlineDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCKlineDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCKlineDlg::OnBnClickedOgl()
{
	MyOglDrawDlg *pTD = new MyOglDrawDlg();
	pTD->Create(IDD_OGL);
	pTD->ShowWindow(SW_SHOWNORMAL);
}

void CMFCKlineDlg::OnLvnItemchangedList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	POSITION curPos = m_list.GetFirstSelectedItemPosition();
	const char Open[4][8] = { "K-line","Qt","WPF","OK" };
	int N = 0;
	CLoginDlg logdlg;
	CMFCKlineDlg newdlg;
	if(!curPos)
		::SendMessage(m_hBottom, SB_SETTEXT, 0, (LPARAM)TEXT(Open[3]));
	else
	{
		while (curPos)
		{
			if(dig)
				if((int)curPos > item && curPos != (POSITION)0x00000001)
			{
				MessageBox("启动错误");
				OnCancel();
				item = 65535;
				dig = true;
				newdlg.DoModal();
				continue;
			}
			if (item == 65535 && curPos == (POSITION)0x00000001) 
			{
				logdlg.DoModal();
				dig = false;
				item = N;
			}
			N = item = m_list.GetNextSelectedItem(curPos);
			::SendMessage(m_hBottom, SB_SETTEXT, 0, (LPARAM)TEXT(Open[item]));
			if (strcmp(Open[item], "Qt") == 0)
				m_Mod.FloatDrift("Qt框架之K线图");
		}
	}
	*pResult = 0;
}

void CMFCKlineDlg::InitMap()
{
	m_Ogl.stockmap.insert(std::pair<int, OGLKview::Strmap>(0, { "SH600747", "大连控股" }));
	m_Ogl.stockmap.insert(std::pair<int, OGLKview::Strmap>(1, { "1", "File1" }));
}

void CMFCKlineDlg::SetList()
{
	RECT m_rect;
	m_list.GetClientRect(&m_rect);
	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_list.InsertColumn(0, _T("打开方式"), 0, m_rect.right / 6);
	m_list.InsertColumn(1, _T("文件"), 0, m_list.GetColumnWidth(0));
	m_list.InsertColumn(2, _T("内容"), 0, m_list.GetColumnWidth(0));
	m_list.InsertColumn(3, _T("..."), 0, m_list.GetColumnWidth(0));
	m_list.InsertItem(0, _T("MFC"));
	m_list.InsertItem(1, _T("QT"));
	m_list.InsertItem(2, _T("WPF"));
	m_list.SetItemText(0, 1, m_Ogl.stockmap[1].info.c_str()); 
	m_list.SetItemText(0, 2, m_Ogl.stockmap[0].info.c_str());
}

void CMFCKlineDlg::OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	POSITION curpos = m_list.GetFirstSelectedItemPosition();
	int item;
	while(curpos)
	{
		item = m_list.GetNextSelectedItem(curpos);
		TRACE1("Item %d was selected!\n", item);
		::PostMessage(this->m_hWnd, WM_MSG_LIST, 0, (LPARAM)item);
	}
	*pResult = 0;
}

void CMFCKlineDlg::SetBottom()
{
	m_hBottom = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER,
		TEXT("OK"),
		GetSafeHwnd(),
		ID_STATUS);
	int pint[4] = { 100,200,350,-1 };
	::SendMessage(m_hBottom, SB_SETPARTS, 4, (LPARAM)pint);
}

LRESULT CMFCKlineDlg::OnItemMsg(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case 0:
		OpenGL(lParam);
		break;
	case 1:
		OpenQt();
		break;
	case 2:
		OpenWpf();
		break;
	default:
		break;
	}
	return LRESULT();
}

bool CMFCKlineDlg::OpenGL(int item)
{
	for (int i = 0; i <= m_list.GetHeaderCtrl()->GetItemCount(); ++i)
	{
		if (m_list.GetItemText(item, i) == m_Ogl.stockmap[item].info.c_str())
			OnBnClickedOgl();
	}
	return true;
}

void CMFCKlineDlg::OpenWpf()
{
	WpfHost::WpfWindow = gcnew WpfKline::MainWindow();
	WpfHost::WpfWindow->ShowDialog();
}

void CMFCKlineDlg::OpenQt()
{
	HWND h_Wnd=::FindWindow(NULL,_T("QtKline"));
	CDialog MessageBox(_T("Qt"));
	OpenQtexe();
}

BOOL CALLBACK EnumChildWindowCallBack(HWND hWnd, LPARAM lParam)
{
	DWORD dwPid = 0;
	GetWindowThreadProcessId(hWnd, &dwPid);  
	if (dwPid == lParam)   
	{
		printf("0x%08X    ", hWnd);  
		TCHAR buf[WINDOW_TEXT_LENGTH];
		SendMessage(hWnd, WM_GETTEXT, WINDOW_TEXT_LENGTH, (LPARAM)buf);
		wprintf(L"%s/n", buf);
		EnumChildWindows(hWnd, EnumChildWindowCallBack, lParam);
	}
	return TRUE;
}

BOOL CALLBACK EnumWindowCallBack(HWND hWnd, LPARAM lParam)
{
	ProcessWindow *pProcessWindow = (ProcessWindow *)lParam;

	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);

	if (pProcessWindow->dwProcessId == dwProcessId && IsWindowVisible(hWnd) && GetParent(hWnd) == NULL)
	{
		pProcessWindow->hwndWindow = hWnd;
	}
	return TRUE;
}

int CMFCKlineDlg::OpenQtexe()
{
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	HWND hChildWnd = NULL;
	//::GetCurrentDirectory(300, (LPTSTR)path);
	//strcat
	CHAR fulPth[256] = { NULL };
	GetModuleFileName(NULL, fulPth, sizeof(fulPth));
	CString dirPth((LPCSTR)fulPth);
	dirPth = dirPth.Left(dirPth.ReverseFind(_T('\\')));
	dirPth = dirPth.Left(dirPth.ReverseFind(_T('\\')));
	LPTSTR cmdline = new TCHAR[dirPth.GetLength() + 40];
	_tcscpy(cmdline,TEXT(dirPth + "\\QtKline\\Win32\\Debug\\QtKline.exe"));
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = TRUE;
	if (::CreateProcess(
		cmdline,
		"", //Unicode版本此参数不能为常量字符串
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi))
	{
		window.dwProcessId = pi.dwProcessId;
  
		EnumWindows(EnumWindowCallBack, (LPARAM)&window);
		WaitForInputIdle(pi.hProcess, 500);

		::CloseHandle(pi.hThread);
		::CloseHandle(pi.hProcess);
	}
	else
	{
		int error = GetLastError();
		printf("error code:%d/n", error);
	}
	delete cmdline;
	//while (!hChildWnd)
	//{
	//	hChildWnd = ::FindWindow(/*0 & , */"QtKline", NULL);
	//	if (hChildWnd != NULL)
	//		break;
	//}
	//HWND hQt = ::FindWindow("QtKline", NULL);
	if (hChildWnd = ::FindWindow(/*0 & , */"QtKline", NULL))
	{
		LONG style = GetWindowLong(hChildWnd, GWL_STYLE);
		style &= ~WS_CAPTION;
		style &= ~WS_THICKFRAME;
		style |=WS_CHILD;  
		SetWindowLong(hChildWnd, GWL_STYLE, style);
		CWnd* pWnd = FromHandle(hChildWnd);
		pWnd->SetParent(this);
		//::SetWindowPos(hChildWnd, HWND_TOPMOST, NULL, NULL, NULL, NULL, SWP_NOSIZE | SWP_NOMOVE);
		//pWnd->SetActiveWindow();
		//pWnd->SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		pWnd->ShowWindow(SW_SHOW);
	}
	return 0;
}


void CMFCKlineDlg::OnBnClickedOK()
{
	CDialogEx::OnOK();
}
