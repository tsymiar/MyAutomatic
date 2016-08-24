#pragma once
#include <afxwin.h>
#include <WinUser.h>
// CMyBkgnd 将模板函数声明与实现写在同一文件内
template <typename Myclass> class CMySet : public Myclass 
{
public:
	CMySet<Myclass>() {};
	virtual ~CMySet<Myclass>() {};
	
	void CommSet(COLORREF color);
};

template <typename Myclass>
void CMySet<Myclass>::CommSet(COLORREF color)
{

}


