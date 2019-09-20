// CCustomCtl.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCKline.h"
#include "CCustomCtl.h"

// CMyEdit 项目

void CMyEdit::FillEdit(CString& tmp)
{
    CString sTemp;
    GetWindowText(sTemp);
    if (sTemp.IsEmpty())
        sTemp += (tmp + "\r\n");
    else
        sTemp += ("\r\n" + tmp);
    SetWindowText(sTemp);
    LineScroll(GetLineCount(), 0);
}

void CMyEdit::FillEdit(const char tmp[], int hexlen)
{
    if (this->m_hWnd == NULL)return;
    CString sTmp;
    GetWindowText(sTmp);
    if (sTmp.GetLength() >= 0x7f0)
        sTmp = "";
    char* temp = new char[2048];
    if (sTmp.IsEmpty())
    {
        sprintf_s(temp, 1024, "%s", tmp);
    }
    else
        if (hexlen > 0)
        {
            unsigned char* uTmp = (unsigned char*)tmp;
            for (int i = 0; i < hexlen; i++)
            {
                if (i == 0)
                    sprintf_s(temp, 1024, "%02X ", uTmp[0]);
                if (!(i % 16))
                    sprintf_s(temp, 1024, "%s %02X\n", (char*)&sTmp, uTmp[i]);
                else
                    sprintf_s(temp, 1024, "%s %02X", (char*)&sTmp, uTmp[i]);
            }
        }
        else
        {
            sprintf_s(temp, 1024, "%s\r\n%s", (char*)&sTmp, tmp);
        }
    SetWindowText((LPCTSTR)temp);
    delete[] temp;
    LineScroll(GetLineCount(), 0);
}

// CMyMenu 项目

void CMyMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    // TODO: 添加您的代码以确定指定项的大小
    lpMeasureItemStruct->itemWidth = NUM_ITEM_WIDTH;
    //ItemInfo *item;
    //item = (ItemInfo*)lpMeasureItemStruct->itemData;
    //if(item->m_itemState == 0)
    if (lpMeasureItemStruct->itemID == 0)
    {
        lpMeasureItemStruct->itemHeight = NUM_SEPARATOR_SPACE;
    }
    else
    {
        lpMeasureItemStruct->itemHeight = NUM_ITEM_HEIGHT;
    }
}

void CMyMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    // TODO: 添加您的代码以绘制指定项
    CString strText;
    CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC); //获取菜单项的设备句柄 
    ItemInfo *item = reinterpret_cast<ItemInfo*>(lpDrawItemStruct->itemData);
    CRect rect(lpDrawItemStruct->rcItem);

    if (item->m_itemState == 0)//分隔条
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

    if (item->m_icon != NULL)
    {
        DrawIconEx(pDC->m_hDC, rect.left + 7, rect.top + 16, item->m_icon, 16, 16, 0, NULL, DI_NORMAL);
    }
    //文字字体和字号设置
    LOGFONT fontInfo;
    pDC->GetCurrentFont()->GetLogFont(&fontInfo);

    fontInfo.lfHeight = 20;
    lstrcpy(fontInfo.lfFaceName, _T("Arial"));
    CFont fontCh;
    fontCh.CreateFontIndirect(&fontInfo);
    pDC->SelectObject(&fontCh);

    if (item->m_itemState == -1)//子菜单
    {
        pDC->TextOut(rect.left + 36, rect.top + 13, item->m_text, item->m_text.GetLength());
        ::ExcludeClipRect(pDC->m_hDC, rect.right - 15, rect.top, rect.right, rect.bottom);
        DrawIconEx(pDC->m_hDC, rect.right - 40, rect.top + 7, AfxGetApp()->LoadIconA(IDI_ICON), 32, 32, 1, NULL, DI_NORMAL);
    }
    else
    {
        pDC->TextOut(rect.left + 36, rect.top + 13, item->m_text, item->m_text.GetLength());
        fontInfo.lfHeight = 16;
        CFont fontEn;
        lstrcpy(fontInfo.lfFaceName, _T("Arial"));
        fontEn.CreateFontIndirect(&fontInfo);
        pDC->SelectObject(&fontEn);
        pDC->TextOut(rect.left + 86, rect.top + 16, item->m_shortcut, item->m_shortcut.GetLength());
    }
    m_InfoList.AddTail(item);
}

void CMyMenu::AppendItem(UINT id, CString strText, CString strShortcut, UINT iconID, UINT nFlags)
{
    ItemInfo *item = new ItemInfo;
    item->m_id = id;
    if (iconID == 0)
    {
        item->m_icon = NULL;
    }
    else
    {
        item->m_icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(iconID), IMAGE_ICON, 16, 16, 0);
        //item->m_icon = AfxGetApp ()- >LoadIconA(iconID) ;
    }
    item->m_text = strText;
    item->m_shortcut = strShortcut;
    item->m_itemState = 1;
    nFlags |= MF_OWNERDRAW;
    item->m_nFlag = nFlags;
    m_InfoList.AddTail(item);
    CMenu::AppendMenuA(nFlags, item->m_id, (LPCTSTR)item);
}

//子菜单
void CMyMenu::AppendSubMenu(UINT id, CMyMenu* subMenu, CString strText, UINT iconID, UINT nFlags)
{
    ItemInfo *item = new ItemInfo;
    item->m_id = id;
    if (iconID == 0)
    {
        item->m_icon = NULL;
    }
    else
    {
        item->m_icon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(iconID), IMAGE_ICON, 16, 16, 0);
    }
    item->m_text = strText;
    item->m_shortcut = "";
    item->m_itemState = -1;
    nFlags |= MF_POPUP | MF_OWNERDRAW;
    item->m_nFlag = nFlags;

    m_InfoList.AddTail(item);
    CMenu::AppendMenu(nFlags, (UINT)subMenu->GetSafeHmenu(), (LPCTSTR)item);
}

//分隔符
void CMyMenu::AppendSeparator(UINT nID, UINT nFlags)
{
    ItemInfo *item = new ItemInfo;
    item->m_id = nID;
    item->m_icon = NULL;
    item->m_text = "";
    item->m_shortcut = "";
    item->m_itemState = 0;
    nFlags |= MF_SEPARATOR | MF_OWNERDRAW;
    item->m_nFlag = nFlags;
    m_InfoList.AddTail(item);
    CMenu::AppendMenu(nFlags, 0, (LPCTSTR)item);
}
