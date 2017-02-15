// IMhideWndDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCKline.h"
#include "IMhideWndDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//收缩模式
#define HM_NONE     0   //不收缩
#define HM_TOP      1   //置顶收缩
#define HM_BOTTOM   2   //置底收缩
#define HM_LEFT     3   //靠左收缩
#define HM_RIGHT    4   //靠右收缩

#define CM_ELAPSE   200 //检测鼠标是否离开窗口的时间间隔
#define HS_ELAPSE   5   //隐藏或显示过程每步的时间间隔
#define HS_STEPS    10  //隐藏或显示过程分成多少步

#define INTERVAL    20  //触发粘附时鼠标与屏幕边界的最小间隔,单位为象素
#define INFALTE     10  //触发收缩时鼠标与窗口边界的最小间隔,单位为象素
#define MINCX       200 //窗口最小宽度
#define MINCY       400 //窗口最小高度

IMPLEMENT_DYNAMIC(CIMhideWndDlg, CDialogEx)

CIMhideWndDlg::CIMhideWndDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_IMHIDEWND, pParent)
{
	//{{AFX_DATA_INIT(CIMhideWndDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32

	m_isSizeChanged = FALSE;
	m_isSetTimer = FALSE;
	m_hsFinished = TRUE;
	m_hiding = FALSE;

	m_oldWndHeight = MINCY;
	m_taskBarHeight = 30;
	m_edgeHeight = 0;
	m_edgeWidth = 0;
	m_hideMode = HM_NONE;
}

CIMhideWndDlg::~CIMhideWndDlg()
{
}

void CIMhideWndDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TIMER, m_timeStatus);
}


BEGIN_MESSAGE_MAP(CIMhideWndDlg, CDialogEx)
	//{{AFX_MSG_MAP(CIMhideWndDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_NCHITTEST()
	ON_WM_TIMER()
	ON_WM_SIZING()
	ON_WM_CREATE()
	ON_WM_MOVING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void get_Rtn(void* lp) {
	int feedback;
	char rcv[128];
	char info[256];
	struct LPR* lpr = (struct LPR*)lp;
	CIMhideWndDlg *dlg = (CIMhideWndDlg*)lpr->p;
	do {
		feedback = recv(lpr->sock, rcv, 256, 0);
		if (!(feedback == 256)) {
			Sleep(100);
			dlg->m_timeStatus.SetWindowText("connection lost\n");
			exit(0);
		};
		EnterCriticalSection(&lpr->wrcon);
		switch (rcv[1])
		{
		case 10:
			//sprintf(info, "join to gruop %s successfully\n", (lastgroup + 8));
			break;
		case 11:
			//sprintf(info, "join to gruop %s rejected :%s\n", (lastgroup + 8), (cmd + 32));
			break;
		case 12:
			//sprintf(info, "create gruop %s successfully\n", (lastgroup + 8));
			break;
		case 13:
			//sprintf(info, "create gruop %s rejected\n", (lastgroup + 8));
			break;
		case 14:
			//sprintf(info, "leave gruop %s successfully\n", (lastgroup + 8));
			break;
		case 20:
			sprintf(info, "%s\n", (rcv + 8));
			break;
		case 21:
			sprintf(info, "%s\n", (rcv + 32));
			break;
		case 22:
			sprintf(info, "info of %s\n %s \n", (rcv + 8), (rcv + 32));
			break;
		case 30:
			//sprintf(info, "you said to %s : %s\n", (lastmsg + 8), (lastmsg + 32));
			break;
		case 31:
			sprintf(info, "received from %s:%s\n", (rcv + 8), (rcv + 32));
			break;
		case 32:
			//sprintf(info, "talk to %s failed\n", (lastmsg + 8));
			break;
		case 122:
			sprintf(info, "other info %d\n", rcv[1]);
			break;
		case 123:
			sprintf(info, "password changed successfully\n");
			break;
		default:
			sprintf(info, "other info %d\n", rcv[1]);
			break;
		}
		dlg->m_timeStatus.SetWindowText(info);
		LeaveCriticalSection(&lpr->wrcon);
	} while (1);
};

int CIMhideWndDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	//获得任务栏高度
	CWnd* p = this->FindWindow("Shell_TrayWnd", NULL);
	if (p != NULL)
	{
		CRect tRect;
		p->GetWindowRect(tRect);
		m_taskBarHeight = tRect.Height();
	}

	//修改风格使得他不在任务栏显示
	ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);
	//去掉关闭按键(如果想画3个按键的话)
	//ModifyStyle(WS_SYSMENU,NULL);
	//获得边缘高度和宽度
	m_edgeHeight = GetSystemMetrics(SM_CYEDGE);
	m_edgeWidth = GetSystemMetrics(SM_CXFRAME);

	//开启聊天
	int err = InitChat("127.0.0.1");
	StartChat(err, get_Rtn);
	CloseChat();
	return 0;
}

BOOL CIMhideWndDlg::SetWindowPos(const CWnd* pWndInsertAfter, LPCRECT pCRect, UINT nFlags)
{
	return CDialog::SetWindowPos(pWndInsertAfter, pCRect->left, pCRect->top,
		pCRect->right - pCRect->left, pCRect->bottom - pCRect->top, nFlags);
}

LRESULT CIMhideWndDlg::OnNcHitTest(CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CString str;
	str.Format("Mouse (%d,%d)", point.x, point.y);
	//GetDlgItem(IDC_TIMER)->SetWindowText(str);
	if (m_hideMode != HM_NONE && !m_isSetTimer &&
		//防止鼠标超出屏幕右边时向右边收缩造成闪烁
		point.x < GetSystemMetrics(SM_CXSCREEN) + INFALTE)
	{   //鼠标进入时,如果是从收缩状态到显示状态则开启Timer
		SetTimer(1, CM_ELAPSE, NULL);
		m_isSetTimer = TRUE;

		m_hsFinished = FALSE;
		m_hiding = FALSE;
		SetTimer(2, HS_ELAPSE, NULL); //开启显示过程
	}
	return CDialog::OnNcHitTest(point);
}

void CIMhideWndDlg::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == 1)
	{
		POINT curPos;
		GetCursorPos(&curPos);

		CString str;
		str.Format("Timer On\n\r(%d,%d)", curPos.x, curPos.y);
		//GetDlgItem(IDC_TIMER)->SetWindowText(str);

		CRect tRect;
		//获取此时窗口大小
		GetWindowRect(tRect);
		//膨胀tRect,以达到鼠标离开窗口边沿一定距离才触发事件
		tRect.InflateRect(INFALTE, INFALTE);

		if (!tRect.PtInRect(curPos)) //如果鼠标离开了这个区域
		{
			KillTimer(1); //关闭检测鼠标Timer
			m_isSetTimer = FALSE;
			//GetDlgItem(IDC_TIMER)->SetWindowText("Timer Off");

			m_hsFinished = FALSE;
			m_hiding = TRUE;
			SetTimer(2, HS_ELAPSE, NULL); //开启收缩过程
		}
	}

	if (nIDEvent == 2)
	{
		if (m_hsFinished) //如果收缩或显示过程完毕则关闭Timer	  
			KillTimer(2);
		else
			m_hiding ? DoHide() : DoShow();
	}

	CDialog::OnTimer(nIDEvent);
}

void CIMhideWndDlg::OnSysCommand(UINT nID, LPARAM lparam)
{
	if (nID == SC_CLOSE)
	{
		DestroyWindow();
		//AfxDump << "OnSysCommand\n";
	}
	CDialog::OnSysCommand(nID, lparam);
}

void CIMhideWndDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	FixMoving(fwSide, pRect); //修正pRect
	CDialog::OnMoving(fwSide, pRect);
}

void CIMhideWndDlg::DoHide()
{
	if (m_hideMode == HM_NONE)
		return;

	CRect tRect;
	GetWindowRect(tRect);

	INT height = tRect.Height();
	INT width = tRect.Width();

	INT steps = 0;

	switch (m_hideMode)
	{
	case HM_TOP:
		steps = height / HS_STEPS;
		tRect.bottom -= steps;
		if (tRect.bottom <= m_edgeWidth)
		{   //你可以把下面一句替换上面的 ...+=|-=steps 达到取消抽屉效果
			//更好的办法是添加个BOOL值来控制,其他case同样.
			tRect.bottom = m_edgeWidth;
			m_hsFinished = TRUE;  //完成隐藏过程
		}
		tRect.top = tRect.bottom - height;
		break;
	case HM_BOTTOM:
		steps = height / HS_STEPS;
		tRect.top += steps;
		if (tRect.top >= (GetSystemMetrics(SM_CYSCREEN) - m_edgeWidth))
		{
			tRect.top = GetSystemMetrics(SM_CYSCREEN) - m_edgeWidth;
			m_hsFinished = TRUE;
		}
		tRect.bottom = tRect.top + height;
		break;
	case HM_LEFT:
		steps = width / HS_STEPS;
		tRect.right -= steps;
		if (tRect.right <= m_edgeWidth)
		{
			tRect.right = m_edgeWidth;
			m_hsFinished = TRUE;
		}
		tRect.left = tRect.right - width;
		tRect.top = -m_edgeHeight;
		tRect.bottom = GetSystemMetrics(SM_CYSCREEN) - m_taskBarHeight;
		break;
	case HM_RIGHT:
		steps = width / HS_STEPS;
		tRect.left += steps;
		if (tRect.left >= (GetSystemMetrics(SM_CXSCREEN) - m_edgeWidth))
		{
			tRect.left = GetSystemMetrics(SM_CXSCREEN) - m_edgeWidth;
			m_hsFinished = TRUE;
		}
		tRect.right = tRect.left + width;
		tRect.top = -m_edgeHeight;
		tRect.bottom = GetSystemMetrics(SM_CYSCREEN) - m_taskBarHeight;
		break;
	default:
		break;
	}

	SetWindowPos(&wndTopMost, tRect);
}

void CIMhideWndDlg::DoShow()
{
	if (m_hideMode == HM_NONE)
		return;

	CRect tRect;
	GetWindowRect(tRect);
	INT height = tRect.Height();
	INT width = tRect.Width();

	INT steps = 0;

	switch (m_hideMode)
	{
	case HM_TOP:
		steps = height / HS_STEPS;
		tRect.top += steps;
		if (tRect.top >= -m_edgeHeight)
		{   //可以把下面一句替换上面的 ...+=|-=steps 达到取消抽屉效果
			//更好的办法是添加个BOOL值来控制,其他case同样.
			tRect.top = -m_edgeHeight;
			m_hsFinished = TRUE;  //完成显示过程
		}
		tRect.bottom = tRect.top + height;
		break;
	case HM_BOTTOM:
		steps = height / HS_STEPS;
		tRect.top -= steps;
		if (tRect.top <= (GetSystemMetrics(SM_CYSCREEN) - height))
		{
			tRect.top = GetSystemMetrics(SM_CYSCREEN) - height;
			m_hsFinished = TRUE;
		}
		tRect.bottom = tRect.top + height;
		break;
	case HM_LEFT:
		steps = width / HS_STEPS;
		tRect.right += steps;
		if (tRect.right >= width)
		{
			tRect.right = width;
			m_hsFinished = TRUE;
		}
		tRect.left = tRect.right - width;
		tRect.top = -m_edgeHeight;
		tRect.bottom = GetSystemMetrics(SM_CYSCREEN) - m_taskBarHeight;
		break;
	case HM_RIGHT:
		steps = width / HS_STEPS;
		tRect.left -= steps;
		if (tRect.left <= (GetSystemMetrics(SM_CXSCREEN) - width))
		{
			tRect.left = GetSystemMetrics(SM_CXSCREEN) - width;
			m_hsFinished = TRUE;
		}
		tRect.right = tRect.left + width;
		tRect.top = -m_edgeHeight;
		tRect.bottom = GetSystemMetrics(SM_CYSCREEN) - m_taskBarHeight;
		break;
	default:
		break;
	}

	SetWindowPos(&wndTopMost, tRect);
}

void CIMhideWndDlg::FixMoving(UINT fwSide, LPRECT pRect)
{
	POINT curPos;
	GetCursorPos(&curPos);
	INT screenHeight = GetSystemMetrics(SM_CYSCREEN);
	INT screenWidth = GetSystemMetrics(SM_CXSCREEN);
	INT height = pRect->bottom - pRect->top;
	INT width = pRect->right - pRect->left;

	if (curPos.y <= INTERVAL)
	{   //粘附在下边
		pRect->bottom = height - m_edgeHeight;
		pRect->top = -m_edgeHeight;
		m_hideMode = HM_BOTTOM;
	}
	else if (curPos.y >= (screenHeight - INTERVAL - m_taskBarHeight))
	{   //粘附在上边
		pRect->top = screenHeight - m_taskBarHeight - height;
		pRect->bottom = screenHeight - m_taskBarHeight;
		m_hideMode = HM_TOP;
	}
	else if (curPos.x < INTERVAL)
	{	//粘附在左边	
		if (!m_isSizeChanged)
		{
			CRect tRect;
			GetWindowRect(tRect);
			m_oldWndHeight = tRect.Height();
		}
		pRect->right = width;
		pRect->left = 0;
		pRect->top = -m_edgeHeight;
		pRect->bottom = screenHeight - m_taskBarHeight;
		m_isSizeChanged = TRUE;
		m_hideMode = HM_LEFT;
	}
	else if (curPos.x >= (screenWidth - INTERVAL))
	{   //粘附在右边
		if (!m_isSizeChanged)
		{
			CRect tRect;
			GetWindowRect(tRect);
			m_oldWndHeight = tRect.Height();
		}
		pRect->left = screenWidth - width;
		pRect->right = screenWidth;
		pRect->top = -m_edgeHeight;
		pRect->bottom = screenHeight - m_taskBarHeight;
		m_isSizeChanged = TRUE;
		m_hideMode = HM_RIGHT;
	}
	else
	{   //不粘附
		if (m_isSizeChanged)
		{   //如果收缩到两边,则拖出来后会变回原来大小
			//在"拖动不显示窗口内容下"只有光栅变回原来大小
			pRect->bottom = pRect->top + m_oldWndHeight;
			m_isSizeChanged = FALSE;
		}
		if (m_isSetTimer)
		{   //如果Timer开启了,则关闭之
			if (KillTimer(1) == 1)
				m_isSetTimer = FALSE;
		}
		m_hideMode = HM_NONE;
		GetDlgItem(IDC_TIMER)->SetWindowText("Timer off");
	}
}
