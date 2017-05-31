#include <iostream>
#include <string>
#include <list>
#include <gdiplus.h>

using namespace Gdiplus;

class ScreenSharing
{
private:
	enum { MSG_IMGEHEAD = 1 };
	struct stMessage
	{
		int    iMessageType;
		SIZE_T len;
		int    ptx;
		int    pty;
	};
	struct Adrr
	{
		int  iport;
		std::string szIp;
	};
	typedef  std::list<Adrr>m_AdrrList;
	m_AdrrList m_list;
	SOCKET PrimaryUDP;
	int TransImage(BYTE* pbyImage, SIZE_T size, int ptx, int pty);
public:
	void ScrnToBmp();
	int ScrnUdpClient();
};