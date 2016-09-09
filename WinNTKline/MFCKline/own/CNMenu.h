#pragma once
#include <Resource.h>
#include <afxtempl.h>
// CNMenu 命令目标

#define NUM_ITEM_WIDTH 90
#define NUM_ITEM_HEIGHT 40
#define NUM_SEPARATOR_SPACE 40
#define NUM_SEPARATOR_HEIGHT 20
#define COLOR_BK RGB(0,0,0)
#define COLOR_SEPARAROR RGB(96,96,96)
#define COLOR_DISABLE RGB(0,0,0)
#define COLOR_TEXT RGB(255,255,255)
#define COLOR_SEL RGB(0,0,0)

class CNMenu : public CMenu
{
	struct ItemInfo 
	{
		int m_id;
		int m_nFlag;
		int m_itemState;
		HICON m_icon;
		CString m_text;
		CString m_shortcut;
	};
public:
	CNMenu() {};
	virtual ~CNMenu() {};
	CList<ItemInfo*> m_InfoList;
	ItemInfo *info = new ItemInfo;
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
};

inline void CNMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	// TODO: 添加您的代码以确定指定项的大小
	lpMeasureItemStruct->itemWidth = NUM_ITEM_WIDTH;
	//ItemInfo *info;
	//info = (ItemInfo*)lpMeasureItemStruct->itemData;
	//if(info->m_itemState == 0)
	if (lpMeasureItemStruct->itemID == 0)
	{
		lpMeasureItemStruct->itemHeight = NUM_SEPARATOR_SPACE;
	}
	else
	{
		lpMeasureItemStruct->itemHeight = NUM_ITEM_HEIGHT;
	}
}

inline void CNMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: 添加您的代码以绘制指定项
	CString strText;
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC); //获取菜单项的设备句柄 
	ItemInfo *info = (ItemInfo*)lpDrawItemStruct->itemData;
	CRect rect(lpDrawItemStruct->rcItem);

	if (info->m_itemState == 0)//分隔条
	{
		pDC->FillSolidRect(rect, COLOR_BK);
		CRect apart = rect;
		apart.top = apart.Height() / 2 + apart.top;
		apart.bottom = apart.top + NUM_SEPARATOR_HEIGHT;
		apart.left += 5;
		apart.right -= 5;

		pDC->Draw3dRect(apart, COLOR_SEPARAROR, COLOR_SEPARAROR);//RGB(64,0,128));

		return;
	}

	if (lpDrawItemStruct->itemState & ODS_GRAYED)
	{
		pDC->FillSolidRect(rect, COLOR_BK);
		pDC->SetTextColor(COLOR_DISABLE);
	}
	else if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		//在菜单项上自绘矩形框的背景颜色 
		pDC->FillSolidRect(rect, COLOR_SEL);
		//设置文字颜色
		pDC->SetTextColor(COLOR_TEXT);
	}
	else
	{
		pDC->FillSolidRect(rect, COLOR_BK);
		pDC->SetTextColor(COLOR_TEXT);
	}
	pDC->SetBkMode(TRANSPARENT);

	if (info->m_icon != NULL)
	{
		DrawIconEx(pDC->m_hDC, rect.left + 7, rect.top + 16, info->m_icon, 16, 16, 0, NULL, DI_NORMAL);
	}
	//文字字体和字号设置
	LOGFONT fontInfo;
	pDC->GetCurrentFont()->GetLogFont(&fontInfo);

	fontInfo.lfHeight = 20;
	lstrcpy(fontInfo.lfFaceName, _T("Arial"));
	CFont fontCh;
	fontCh.CreateFontIndirect(&fontInfo);
	pDC->SelectObject(&fontCh);

	if (info->m_itemState == -1)//子菜单
	{
		pDC->TextOut(rect.left + 36, rect.top + 13, info->m_text, info->m_text.GetLength());
		::ExcludeClipRect(pDC->m_hDC,rect.right-15,rect.top,rect.right,rect.bottom);
		DrawIconEx(pDC->m_hDC,rect.right-40,rect.top+7,AfxGetApp()->LoadIconA(IDI_ICON),32,32,1,NULL,DI_NORMAL);        
	}
	else
	{
		pDC->TextOut(rect.left + 36, rect.top + 13, info->m_text, info->m_text.GetLength());
		fontInfo.lfHeight = 16;
		CFont fontEn;
		lstrcpy(fontInfo.lfFaceName, _T("Arial"));
		fontEn.CreateFontIndirect(&fontInfo);
		pDC->SelectObject(&fontEn);
		pDC->TextOut(rect.left + 86, rect.top + 16, info->m_shortcut, info->m_shortcut.GetLength());
	}
	m_InfoList.AddTail(info);
}
