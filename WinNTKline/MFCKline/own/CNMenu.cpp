// NMenu.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCKline.h"
#include "CNMenu.h"
// CNMenu

CNMenu::CNMenu()
{
}

CNMenu::~CNMenu()
{
}
//项目
void CNMenu::AppendItem(UINT id, CString strText, CString strShortcut, UINT iconID, UINT nFlags)
{
	ItemInfo *info = new ItemInfo;
	info->m_id = id;
	if (iconID == 0)
	{
		info->m_icon = NULL;
	}
	else
	{
		info->m_icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(iconID), IMAGE_ICON, 16, 16, 0);
		//info->m_icon = AfxGetApp ()- >LoadIconA(iconID) ;
	}
	info->m_text = strText;
	info->m_shortcut = strShortcut;
	info->m_itemState = 1;
	nFlags |= MF_OWNERDRAW;
	info->m_nFlag = nFlags;
	m_InfoList.AddTail(info);
	CMenu::AppendMenuA(nFlags, info->m_id, (LPCTSTR)info);
}

//子菜单
void CNMenu::AppendSubMenu(UINT id, CNMenu* subMenu, CString strText, UINT iconID, UINT nFlags)
{
	ItemInfo *info = new ItemInfo;
	info->m_id = id;
	if (iconID == 0)
	{
		info->m_icon = NULL;
	}
	else
	{
		info->m_icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(iconID), IMAGE_ICON, 16, 16, 0);
	}
	info->m_text = strText;
	info->m_shortcut = "";
	info->m_itemState = -1;
	nFlags |= MF_POPUP | MF_OWNERDRAW;
	info->m_nFlag = nFlags;

	m_InfoList.AddTail(info);
	CMenu::AppendMenu(nFlags, (UINT)subMenu->GetSafeHmenu(), (LPCTSTR)info);
}

//分隔符
void CNMenu::AppendSeparator(UINT nID, UINT nFlags)
{
	ItemInfo *info = new ItemInfo;
	info->m_id = nID;
	info->m_icon = NULL;
	info->m_text = "";
	info->m_shortcut = "";
	info->m_itemState = 0;
	nFlags |= MF_SEPARATOR | MF_OWNERDRAW;
	info->m_nFlag = nFlags;
	m_InfoList.AddTail(info);
	CMenu::AppendMenu(nFlags, 0, (LPCTSTR)info);
}
