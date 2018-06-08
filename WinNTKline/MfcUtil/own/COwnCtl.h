#if !defined(OWNCTL_H)
#define OWNCTL_H
#include <Resource.h>
#include <afxtempl.h>

// CMyIPCtl 命令目标
class CMyIPCtl : CIPAddressCtrl {
public:void Ip2Str(CString &strIP)
	{
		DWORD  dwIP;
		unsigned  char  *pIP;
		GetAddress(dwIP);
		pIP = (unsigned  char*)&dwIP;
		strIP.Format("%u.%u.%u.%u", *(pIP + 3), *(pIP + 2), *(pIP + 1), *pIP);
	}
};

// CMyEdit 命令目标
class CMyEdit : public CEdit{
public:
	void FillEdit(CString& tmp);
	void FillEdit(const char tmp[], int hexlen);
};

// CMyMenu 命令目标
class CMyMenu : public CMenu
{
#define NUM_ITEM_WIDTH 90
#define NUM_ITEM_HEIGHT 40
#define NUM_SEPARATOR_SPACE 40
#define NUM_SEPARATOR_HEIGHT 20
#define COLOR_BK RGB(0,0,0)
#define COLOR_SEPARAROR RGB(96,96,96)
#define COLOR_DISABLE RGB(0,0,0)
#define COLOR_TEXT RGB(255,255,255)
#define COLOR_SEL RGB(0,0,0)
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
	CMyMenu() {};
	virtual ~CMyMenu() {};
	CList<ItemInfo*> m_InfoList;
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	void AppendItem(UINT id, CString strText, CString strShortcut, UINT iconID, UINT nFlags); 
	void AppendSubMenu(UINT id, CMyMenu* subMenu, CString strText, UINT iconID, UINT nFlags);
	void AppendSeparator(UINT nID, UINT nFlags);
};
#endif
