#include <iostream>
#include <string>
#include <list>
#include <gdiplus.h>
#include <ws2tcpip.h>

using namespace Gdiplus;

class ScreenSharing
{
private:
    enum { MSG_IMGEHEAD = 1 };
    struct stMessage
    {
        int    iMessageType = 0;
        SIZE_T len = 0;
        int    ptx = 0;
        int    pty = 0;
    };
    struct Adrr
    {
        int  iport = 0;
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
