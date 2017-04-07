#pragma once
#include <afxwin.h>
#include <WinUser.h>
// CommSet 将模板函数声明与实现写在同一文件内
template <typename Myclass> class CommSet : public Myclass
{
public:
	CommSet<Myclass>() {};
	virtual ~CommSet<Myclass>() {};
	void ColrSet(COLORREF color);
};

template <typename Myclass>
void CommSet<Myclass>::ColrSet(COLORREF color)
{

}
