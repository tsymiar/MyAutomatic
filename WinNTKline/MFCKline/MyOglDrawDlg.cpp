#include "MyOglDrawDlg.h"

// MyOglDrawDlg.cpp : 

IMPLEMENT_DYNAMIC(MyOglDrawDlg, CDialog)

MyOglDrawDlg::MyOglDrawDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_OGLDLG, pParent)
{
	//m_hIcon = LoadIcon((HINSTANCE)IDR_ICON, "");
}

void MyOglDrawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GLTAB, m_tab);
}

BEGIN_MESSAGE_MAP(MyOglDrawDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_WM_PAINT()
	//ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_SYSCOMMAND()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONDOWN()
	ON_MESSAGE(WM_MSG_OGL, GetOglCmd)
	ON_MESSAGE(WM_MSG_SHOW, OnTaskShow)
	ON_COMMAND(WM_MSG_QUIT, &MyOglDrawDlg::ToQuit)
	ON_COMMAND(WM_MSG_BKG, &MyOglDrawDlg::SetBkg)
	ON_COMMAND(WM_MSG_DEP, &MyOglDrawDlg::Set_5_Deg)
END_MESSAGE_MAP()
///*******************定义本文件内的全局变量*******************///
	MyOglDrawDlg* Mod;//Notify
	OGLKview Ogl;//FillChart
	//OnTaskShow
	NOTIFYICONDATA icon;
	//END OnTaskShow
///**************GetMarkDatatoDraw()**************///
	int		i = 0;
	int		pi = 0;
	int		line = 0;
	int		item0 = 0;
	char*	cot = NULL;
	char*	buff = NULL;
	char*	token = NULL;
	bool	draw5  = false;
	bool	draw10 = false;
	bool	draw20 = false;
	bool	isnext = false;
	char*	div_stock[3] = { NULL };
	char	ma[32]	= { NULL };
	char	dif[32] = { NULL };
	char	dea[32] = { NULL };
	char	rsi[32] = { NULL };
	char	vol[32] = { NULL };
	char	macd[32] = { NULL };
	char	code[64] = { NULL };
	char	exeFullPath[256] = { NULL };
	std::vector<Stock::Rsi> str_rsi;
	std::string tmp = " ";
	std::ifstream Readfile;
	std::ostream;
	WIN32_FIND_DATA fileData;
	Stock::Rsi rise{ 0 }, drop{ 0 }, total{ 0 }, last{ 0 };
	Stock::Sma::MA curma{ 0 }, totma{ 0 }, yma{ 0 };
	Stock::Sma ma20{ 0 }, ma10{ 0 }, ma5{ 0 };
	OGLKview::Point Per = { 0 }, P[4] = { 0 };
	OGLKview::Point p_vol = { 0 }, p_dif = { 0 }, p_dea = { 0 }, p_rsi = { 0 }, p_macd = { 0 };
	OGLKview::Point p_code = { 0.8f,1.189f }, p_stock = { -1.22f,p_code.y };
	OGLKview::Point p_rsi6 = { 0 }, p_rsi12 = { 0 }, p_24 = { 0 };
	OGLKview::Point p_ma5 = { 0 }, p_ma10 = { 0 }, p_ma20 = { 0 };
	OGLKview::Point p_ma5old = { 0 }, p_ma10old = { 0 }, p_ma20old = { 0 };
	OGLKview::Point p_AB6 = { 0 }, p_AB12 = { 0 }, p_AB24 = { 0 };
	OGLKview::Point p_AB6old = { 0 }, p_AB12old = { 0 }, p_AB24old = { 0 };
///**************END GetMarkDatatoDraw**************///
///**************************end**************************///
BOOL MyOglDrawDlg::OnInitDialog()
{
	m_hWnd = this->GetSafeHwnd();
	m_hDC = ::GetDC(m_hWnd/*m_tab.GetSafeHwnd()*/);
	Ogl.SetWindowPixelFormat(m_hDC, m_hWnd);
	OnPaint();
	Ogl.AdjustDraw(W, H);
#ifdef GLTEST
	//test.LoadGLTexture();
	test.Load__qdu(W, H);
#endif
	SetTimer(1, 30, NULL);
	SetWindowPos(FromHandle(GetSafeHwnd()), 0, 0, GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN), SWP_NOZORDER);
	m_hAcc = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(ID_ACCLER));
	CloseHandle((HANDLE)_beginthreadex(NULL, 0, ClientThread, (void*)this, 0, NULL));
	return CDialog::OnInitDialog();
}

void MyOglDrawDlg::OnPaint()
{
	CPaintDC dc(this);
	CRect Recto;
	GetClientRect(&Recto);
	dc.SetViewportOrg(W = Recto.Width(), H = Recto.Height());
}

HCURSOR MyOglDrawDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
#ifdef _UNICODE
void MyOglDrawDlg::OnTimer(UINT_PTR nIDEvent)
#else
void MyOglDrawDlg::OnTimer(UINT nIDEvent)
#endif
{
	switch (nIDEvent)
	{
	case 1://调用绘图函数
		DrawFunc(m_hDC);
		break;
	default:
		break;
	}
	CDialog::OnTimer(nIDEvent);
}

unsigned int __stdcall MyOglDrawDlg::ClientThread(void* pParam)
{ 
	//线程要调用的函数
	int err;
	int j = 0;
	SOCKET clientSock;
	MyOglDrawDlg *m_user = new MyOglDrawDlg();
	WSADATA wsaData;//WSAata存储系统传回的关于WinSocket的信息
	int loo[8];
	CString CStr;
	CString Left;
	char Buf[50];
	char buffer[10];
	WCHAR LPCT[50];
	SOCKADDR_IN addrSrv;
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD(1, 1);//连接两个无符号参数
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		m_user->MessageBox("服务器可能未启动！");
		if (m_user != NULL)
		{
			m_user = NULL;
			delete m_user;
		}
		return 0;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		if (m_user != NULL)
		{
			m_user = NULL;
			delete m_user;
		}
		return 0;
	}
	clientSock = socket(AF_INET, SOCK_STREAM, 0);//AF_INET 表示TCP 连接
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//本机地址,服务器在本机开启
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6001);//设置端口号
	connect(clientSock, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));//连接服务器
	while (1)
	{
		if (recv(clientSock, Buf, 50, 0) == SOCKET_ERROR)
		{
			m_user->MessageBox("登入服务器失败");
			delete m_user;
			return 0;
		}//接收数据并填充到列表
		MultiByteToWideChar(CP_ACP, 0, Buf, strlen(Buf) + 1, LPCT, sizeof(LPCT) / sizeof(LPCT[0]));
		CStr.Format(_T("%s"), LPCT);
		loo[0] = CStr.Find(_T("."));//查找第一个"."位置
		Left = CStr.Left(loo[0]);	//将","左边的值取出
		_ultoa_s(GetCurrentThreadId(), buffer, 10);//当前线程id
		send(clientSock, buffer, strlen(buffer) + 1, 0);//发送数据
		j++;
	}
	closesocket(clientSock);
	WSACleanup();
	delete m_user;
	return 0;
}

NOTIFYICONDATA MyOglDrawDlg::myNotify(HWND O_hWnd)
{
	icon.cbSize = (DWORD)sizeof(NOTIFYICONDATA);
	icon.hWnd = O_hWnd;
	icon.uID = IDR_MAINFRAME;
	icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	icon.uCallbackMessage = WM_MSG_SHOW;
	icon.uTimeout = 3000;
	icon.dwInfoFlags = NIIF_INFO;
	_tcscpy(icon.szTip, "隐藏的对话框");
	_tcscpy(icon.szInfoTitle, "已最小化到托盘区");
	_tcscpy(icon.szInfo, "点击图标恢复，右键弹出菜单。");
	icon.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	return icon;
}

void MyOglDrawDlg::FloatDrift(char* text)
{
	CWindowDC wdc(Mod->FromHandle(::GetDesktopWindow()));
	CFont font;
	wdc.SetBkMode(BKMODE_LAST);
	wdc.SetTextColor(RGB(20, 20, 20));
	//wdc.BitBlt(0,0,
	wdc.LineTo(0, 0);
	font.CreateFont(17, 17, 0, 0, FW_SEMIBOLD, true, false, false,
		CHINESEBIG5_CHARSET, OUT_CHARACTER_PRECIS,
		CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		FF_MODERN, "仿宋");
	wdc.SelectObject(&font);
	wdc.TextOut(400, 400, text, strlen(text));
	::Sleep(100);
	if (text)
	{
		text = NULL;
		delete text;
	}
}

void _stdcall MyOglDrawDlg::DrawFunc(HDC m_hDC)
{
	Ogl.InitGraph();
	Ogl.DrawCoord(Ogl.dlginfo.mouX, Ogl.dlginfo.mouY);
#if !defined(GLTEST)
	GetMarkDatatoDraw();
	chart.FillChart(Ogl.unfurl);
#else
	//test.Load__qdu(W, H);
	test.Model(Ogl.dlginfo.width, Ogl.dlginfo.height, dX, dY);
	test.House(Ogl.dlginfo.width, Ogl.dlginfo.height);
	//test.glTEST(true);
#endif
	Ogl.AdjustDraw(Ogl.dlginfo.width, Ogl.dlginfo.height, Ogl.dlginfo.bkg);
	SwapBuffers(m_hDC);
}

bool MyOglDrawDlg::GetMarkDatatoDraw()
{
	GetModuleFileName(NULL, exeFullPath, sizeof(exeFullPath));
	CString csDirPath((LPCSTR)exeFullPath);
	csDirPath = csDirPath.Left(csDirPath.ReverseFind(_T('\\')));
	std::string sSystemPath = csDirPath.GetBuffer(csDirPath.GetLength());
	csDirPath += "\\*.txt";
	FindFirstFile(csDirPath.GetBuffer(), &fileData);
	std::string sSystemFullPath = sSystemPath + _T("\\") + fileData.cFileName;
	std::string sWritePath = sSystemPath + _T("\\") + _T("MACD.txt");
	const char* filename = sSystemFullPath.c_str();
	const char* writefile = sWritePath.c_str();
	Readfile.open(filename, std::ios::in);
	if (Readfile.fail())
	{
		if (failmsg < 1)
			::PostMessage(m_hWnd, WM_MSG_OGL, 0, (LPARAM)Ogl.index.AllocBuffer("Reading failure"));
		failmsg++;
		return false;
	}
	else {
		while (getline(Readfile, tmp))
		{
			buff = (char*)tmp.c_str();
			token = strtok_s(buff, "/,\t", &cot);
			while (token != NULL)
			{
				markdata.push_back(token);
				token = strtok_s(NULL, "/,\t", &cot);
			}
			if ((markdata.size() <= 7) && (markdata.size()>0))
			{
				if (markdata.size() < 3)
				{
					Ogl.coding = true;	
					buff = markdata[0];	
					title.Format("%s", buff);
					SetWindowText(title);
					i = 0;
					token = strtok_s(buff, " ", &cot);
					while (token != NULL)
					{
						div_stock[i] = token;
						token = strtok_s(NULL, " ", &cot);
						i++;
					}
					Ogl.SwitchViewport(0);
					sprintf(code, "%s(%s)", div_stock[1], div_stock[0]);
					Ogl.DrawKtext(code, p_code, 20, { 1,1,0 }, "Terminal", false);
					sprintf(code, _T("%s(%s)<%s>"), div_stock[1], div_stock[0], div_stock[2]);
					p_code = { -1.22f,p_code.y };
					Ogl.DrawKtext(code, p_code, 12, { 1,1,1 }, "宋体");
					p_code = { 0.8f,1.189f };
					*div_stock = NULL;
				}
				else { 1; }
				markdata.clear();
			}
			else if (markdata.size() > 8)
			{
				Ogl.lastmarket = trademarket;
				trademarket.time.tm_year	= atoi(markdata[0]);
				trademarket.time.tm_mon		= atoi(markdata[1]);
				trademarket.time.tm_mday	= atoi(markdata[2]);
				trademarket.open	= (float)atof(markdata[3]);
				trademarket.high	= (float)atof(markdata[4]);
				trademarket.low		= (float)atof(markdata[5]);
				trademarket.close	= (float)atof(markdata[6]);
				trademarket.amount	= atoi(markdata[7]);
				trademarket.price	= (float)atof(markdata[8]);
				market.push_back(trademarket);
				if (line < Ogl.tinkep.move + Ogl.dlginfo.cycle / Ogl.tinkep.ratio)
				{
					if(line > 0)
						if (pi > 3)
						{
							pi = 0;
							isnext = true;
						}
						if (line <= 3)
						{
							Ogl.dlginfo.line = 1;
						}
						else
							Ogl.dlginfo.line = line - 2;
						if (Ogl.tinkep.ratio == 0)
						{
							P[pi].x += 6.5f;
							P[pi].y /= 2;
						}
					P[pi].x = Ogl.Pxtinker(Ogl.tinkep);
					P[pi].y = (float)atof(markdata[6]);
					ASSERT(_CrtCheckMemory());
					Ogl.dlginfo.line == 1 ? Per = P[0] : Per = P[pi];
					if (line >= Ogl.tinkep.move)
						Ogl.DrawKline(trademarket, Ogl.tinkep);
					Ogl.DrawPoly(Per, P[pi], { 0.f,1.f,0.f });
					Ogl.dlginfo.line == 1 ? Per = P[0] : Per = P[pi];
					if (line % 20 == 0)
					{
						ma20.X = (int)totma._20;
						ma20.M = 1;
						ma20.N = 20;
						if (line == 20)
						{
							curma._20 = totma._20 / 20;
							p_ma20old.x = Per.x;
							p_ma20old.y = p_ma20.y;
						}
						else
						{
						
						}
					}
					pi++;
				}
				markdata.clear();
			}
			else return false;
			line++;
		}
		Ogl.dlginfo.line = line = 0;
	}
	Readfile.close();
	return true;
}
// MyOglDrawDlg 

void MyOglDrawDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	Ogl.dlginfo.mouX = point.x; 
	Ogl.dlginfo.mouY = point.y;
	if (point.x > 980 || point.y < 22)
		Ogl.dlginfo.drawstaff = 0;
	if ((Ogl.dlginfo.leftdown) && (point.x < Ogl.dlginfo.width) && (point.y < Ogl.dlginfo.height))
	{
		Ogl.dlginfo.drawstaff = 0;
		dX = point.x - oldX;
		dY = point.y - oldY;
		float Delta = sqrt(dX*dX + dY*dY);
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		if (Delta >= 6)
		{
			if (oldX < point.x)
			{
				Ogl.tinkep.move--;
			}
			else if (oldX > point.x)
				Ogl.tinkep.move++;
			else 1;
			oldX = (float)point.x;
			oldY = (float)point.y;
		}
	}
	else if ((point.x >= Ogl.dlginfo.width) || (point.y >= Ogl.dlginfo.height))
		Ogl.dlginfo.leftdown = false;
}

BOOL MyOglDrawDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if (zDelta == WHEEL_DELTA)
		Ogl.tinkep.ratio*=2;
	if (zDelta == -WHEEL_DELTA)
	{
		Ogl.tinkep.ratio /= 2;
		if (Ogl.tinkep.ratio < (1.0f / 32))
			Ogl.tinkep.ratio = (1.0f / 32);
	}
	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}

void MyOglDrawDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	Ogl.dlginfo.drawstaff++;
	if (Ogl.dlginfo.drawstaff == 2)
		Ogl.dlginfo.drawstaff = 0;
	Ogl.dlginfo.leftdown = true;
	return CDialog::OnLButtonDown(nFlags, point);
}

void MyOglDrawDlg::OnRButtonDown(UINT nFlags, CPoint point)
{

}

void MyOglDrawDlg::OnSysCommand(UINT nID, LPARAM lparam)
{
	if (SC_MINIMIZE == nID)
	{
#ifndef _WIN64
		NOTIFYICONDATA noti=myNotify(m_hWnd);
		Shell_NotifyIcon(NIM_ADD, &noti);
		ShowWindow(SW_HIDE);
#endif
	}
	else 
		CDialog::OnSysCommand(nID, lparam);
}

LRESULT MyOglDrawDlg::GetOglCmd(WPARAM wparam, LPARAM lparam)
{
	CString cmd;
	char* pMsg = (char*)lparam;
	cmd.Format(_T("%s"),pMsg);
	if (MessageBox(cmd) == IDOK)
		OnClose();
	if (pMsg)
	{
		pMsg = NULL;
		delete pMsg;
	}
	return LRESULT();
}

BOOL MyOglDrawDlg::OnSetCursor(CWnd * pWnd, UINT nHitTest, UINT message)
{
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	if (nHitTest != HTCAPTION)
	{
		if (Ogl.dlginfo.drawstaff)
		{
			if ((Ogl.dlginfo.mouY >= 20) && (Ogl.dlginfo.mouX < 950) && (Ogl.dlginfo.mouY < 380))
				SetCursor(NULL);
		}
		else if((Ogl.dlginfo.mouY==380)&&(Ogl.dlginfo.mouX<950))
			SetCursor(LoadCursor(NULL, IDC_SIZENS));
	}
	return 0;
}

void MyOglDrawDlg::OnSize(UINT nType, int cx, int cy)
{
	Ogl.dlginfo.width = cx;
	Ogl.dlginfo.height = cy;
	CDialog::OnSize(nType, cx, cy);
}

LRESULT MyOglDrawDlg::OnTaskShow(WPARAM wparam, LPARAM lparam)
{
	CMenu menu;
	HMENU hmenu;
	CBitmap bitmap;
	if (lparam != IDR_MAINFRAME)
		switch (lparam)
		{
			case WM_RBUTTONUP:
			{
				LPPOINT lpoint = new tagPOINT;
				::GetCursorPos(lpoint);
				menu.CreatePopupMenu();
				menu.AppendMenu(MF_STRING, /*ID_APP_EXIT*/WM_MSG_QUIT, _T("Exit"));
				SetForegroundWindow();
				bitmap.LoadBitmap(IDB_BITMAP);
				menu.SetMenuItemBitmaps(0, MF_BYPOSITION, &bitmap, &bitmap);
				menu.TrackPopupMenu(TPM_LEFTALIGN, lpoint->x, lpoint->y, this);
				hmenu = menu.Detach();
				menu.DestroyMenu();
				delete lpoint;
			}
				break;
			case WM_LBUTTONDOWN:
				if(IsWindowVisible())
					ShowWindow(SW_HIDE);
				else
				{
					ShowWindow(SW_SHOW);
					Shell_NotifyIcon(NIM_DELETE, &icon);
				}
				break;
			default:
				break;
		}
	return LRESULT();
}

LRESULT CALLBACK MyOglDrawDlg::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DESTROY)
		OnDestroy();
	return CDialog::WindowProc(uMsg, wParam, lParam);
}

BOOL MyOglDrawDlg::PreTranslateMessage(tagMSG * pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (m_hAcc && ::TranslateAccelerator(m_hWnd, m_hAcc, pMsg))
		{
			Ogl.DOS.OpenConsole();
			//Ogl.DOS.ConsoleIOoverload();
		}
		else switch (pMsg->wParam)
		{
			case 'F':
			case 'f':
			{
				Set_5_Deg();
			}
			break;
			case 'M':
			{
				chart.item.time.hour = chart.item.time.min = 9;
				chart.item.pc_ = 99;
				chart.item.mode = 1;
				chart.DrawItem(chart.item, 0);
			}
			break;
			default:	break;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL MyOglDrawDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
//	char buffer[1];
#ifdef _DEBUG
	if (nCode < 0 && nCode != (int)0x8000)
		TRACE1("Implementation Warning: control notification = $%X.\n",
			nCode);
#endif
	if (nCode == CN_COMMAND)
	{
		//for (UINT i = IDR_BKG + 1; i <= IDR_BKG + 5; i++)
		//	if (nID == i)
		//	{
		//		itoa(i - IDR_BKG, buffer, 10);
		//	}
		switch (nID)
		{
		case ID_BKG:
			SetBkg();
			break;
		case ID_DEEP:
			Set_5_Deg();
			break;
		default:
			break;
		}
	}
	else
	{
		//if (_afxThreadState->m_hLockoutNotifyWindow == m_hWnd)
		//	return TRUE;        // locked out - ignore control notification
							// zero IDs for normal commands are not allowed
		if (nID == 0)
			return FALSE;
	}
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void MyOglDrawDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_MENU);
	//CMenu* pPop = menu.GetSubMenu(0);
	//if (pPop != NULL)
	{
		ClientToScreen(&point);
		menu./*pPop->*/TrackPopupMenu(TPM_CENTERALIGN, point.x, point.y, pWnd);
	}
	menu.Detach();
}

void MyOglDrawDlg::SetBkg()
{
	if (Ogl.dlginfo.bkg)
		Ogl.dlginfo.bkg = false;
	else
		Ogl.dlginfo.bkg = true;
}

void MyOglDrawDlg::Set_5_Deg()
{
	if (Ogl.unfurl)
		Ogl.unfurl = false;
	else
		Ogl.unfurl = true;
}

void MyOglDrawDlg::OnClose()
{
	KillTimer(1);
	PostNcDestroy();
}

void MyOglDrawDlg::PostNcDestroy()
{
	delete this;
}

void MyOglDrawDlg::ToQuit()
{
	MyOglDrawDlg::OnClose();
}

MyOglDrawDlg::~MyOglDrawDlg()
{
	Shell_NotifyIcon(NIM_DELETE, &icon);
}