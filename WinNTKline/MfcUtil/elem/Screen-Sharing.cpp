#include "stdafx.h"
#include "Screen-Sharing.h"

using namespace std;

#define HEIGHTPOS 10
#define _WINSOCK_DEPRECATED_NO_WARNINGS

void ScreenSharing::ScrnToBmp()
{
	HWND v_hWnd = ::GetDesktopWindow();	//得到整个屏幕句柄
	HDC   v_hDC = ::GetDC(v_hWnd);	//得到当前屏幕的设备环境句柄
	CRect v_Rect;	//定义一个RECT结构
	::GetWindowRect(v_hWnd, &v_Rect);	//获得窗口的尺寸信息
	HDC   hMemDC = ::CreateCompatibleDC(v_hDC);	//创建兼容的设备环境
	HBITMAP hBmp = ::CreateCompatibleBitmap(v_hDC, v_Rect.right, v_Rect.bottom);	//创建兼容GDI位图
	HGDIOBJ hOld = ::SelectObject(hMemDC, hBmp);	//将位图选入设备环境
	BitBlt(hMemDC, v_Rect.left, v_Rect.top, v_Rect.right, v_Rect.bottom, v_hDC, 0, 0, SRCCOPY);	//当前屏幕设备环境中的颜色数据拷贝到新建的兼容的设备环境中
	HCURSOR hand = GetCursor();	//获取鼠标句柄
	POINT handpos;	//存放鼠标位置
	GetCursorPos(&handpos);	//获得鼠标位置
	DrawIcon(hMemDC, handpos.x, handpos.y, hand);	//在位图中画入鼠标
	BITMAPINFO bh;	//定义DIB位图信息结构
	bh.bmiHeader.biSize = sizeof(bh.bmiHeader);	//位图信息头大小
	bh.bmiHeader.biWidth = v_Rect.right;	//设置位图宽度
	bh.bmiHeader.biHeight = v_Rect.bottom;	//位图高
	bh.bmiHeader.biPlanes = 1;	//位面数
	bh.bmiHeader.biBitCount = 24;	//每个像素位数
	bh.bmiHeader.biCompression = BI_RGB;	//压缩说明:不压缩
	bh.bmiHeader.biSizeImage = 0;	//位图数据大小
	bh.bmiHeader.biXPelsPerMeter = 0;	//水平分辨率=像素/米
	bh.bmiHeader.biYPelsPerMeter = 0;	//垂直分辨率
	bh.bmiHeader.biClrUsed = 0;	//所有调色板项
	bh.bmiHeader.biClrImportant = 0;	//设定所有颜色同样重要
	BYTE* bits = new BYTE[800 * 600 * 3];	//图像大小
	::GetDIBits(hMemDC, hBmp, 0, v_Rect.bottom, NULL, &bh, DIB_RGB_COLORS);	//拷贝位图信息结构到缓冲区
	CFile f("e:/a.bmp", CFile::modeWrite | CFile::modeCreate);	//新建位图文件
	BITMAPFILEHEADER bf;	//定义文件头
	bf.bfSize = sizeof(bf);	//设置文件头大小
	bf.bfOffBits = sizeof(bf) + sizeof(bh.bmiHeader);	//说明从文件头开始到实际图像数据之间的字节偏移量
	bf.bfType = (int)'M' * 256 + 'B';	//文件单位
	bf.bfReserved1 = bf.bfReserved2 = 0;	//保留，必须为0
	f.Write(&bf, sizeof(bf));	//拷贝文件头信息到文件
	f.Write(&bh, 40);
	f.Write(bits, 800 * 600 * 16);	//拷贝图像数据
	f.Close();
	delete bits;
}

int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
	int nRet = -1;
	ImageCodecInfo * pCodecInfo = NULL;
	UINT nNum = 0, nSize = 0;
	GetImageEncodersSize(&nNum, &nSize);
	if (nSize<0)
	{
		return nRet;
	}
	pCodecInfo = new ImageCodecInfo[nSize];
	if (pCodecInfo == NULL)
	{
		return nRet;
	}
	GetImageEncoders(nNum, nSize, pCodecInfo);
	for (UINT i = 0; i<nNum; i++)
	{
		if (wcscmp(pCodecInfo[i].MimeType, format) == 0)
		{
			*pClsid = pCodecInfo[i].Clsid;
			nRet = i;
			delete[] pCodecInfo;
			return nRet;
		}
		else
		{
			continue;
		}
	}
	delete[] pCodecInfo;
	return nRet;
}

int ScreenSharing::TransImage(BYTE* pbyImage, SIZE_T size, int ptx, int pty)
{
	list<Adrr>::iterator it = m_list.begin();
	for (; it != m_list.end(); ++it)
	{
		sockaddr_in rmtsock;
#ifdef _UTILAPIS_
		inet_pton(AF_INET, (*it).szIp.c_str(), (PVOID*)&rmtsock.sin_addr.s_addr);
#else
		rmtsock.sin_addr.S_un.S_addr = inet_addr((*it).szIp.c_str());
#endif
		rmtsock.sin_family = AF_INET;
		rmtsock.sin_port = htons((*it).iport);

		stMessage sendmsg;
		sendmsg.iMessageType = MSG_IMGEHEAD;
		sendmsg.len = size;
		sendmsg.ptx = ptx;
		sendmsg.pty = pty;
		int ret = sendto(PrimaryUDP, (const char *)&sendmsg, sizeof(stMessage), 0, (const sockaddr*)&rmtsock, sizeof(rmtsock));
		if (ret < 0)
		{
			return -1;
		}
		ret = sendto(PrimaryUDP, (const char *)pbyImage, (int)size, 0, (const sockaddr*)&rmtsock, sizeof(rmtsock));
		if (ret < 0)
		{
			return -1;
		}
	}
	return 0;
}

int ScreenSharing::ScrnUdpClient()
{
	HWND hWnd = ::GetDesktopWindow();
	HDC   m_hHackDC = ::GetDC(hWnd);
	CRect m_DestClientRect;
	::GetClientRect(hWnd, &m_DestClientRect);

	Bitmap bmpGameWin(m_DestClientRect.Width(), m_DestClientRect.Height(), PixelFormat32bppARGB);
	if (bmpGameWin.GetLastStatus() != 0)
		return -1;
	Graphics g(&bmpGameWin);

	HDC hDC = g.GetHDC();
	//拷贝目标程序的画面到本端内存位图
	::BitBlt(hDC, 0, 0,
		m_DestClientRect.Width(),
		m_DestClientRect.Height(),
		m_hHackDC, 0, 0, SRCCOPY);

	::ReleaseDC(hWnd, m_hHackDC);
	g.ReleaseHDC(hDC);

	CLSID clsid;
	int nRet = 0;
	nRet = GetEncoderClsid(L"image/jpeg", &clsid); //得到CLSID
												   //  	if(!bmpGameWin.Save(L"c:\\33.jpg",&clsid,NULL) == Ok)
												   //  		return -1;
	int iWidth = bmpGameWin.GetWidth();
	int iHeight = bmpGameWin.GetHeight();
	for (int y = iHeight; y > 0; y -= HEIGHTPOS)
	{
		int nHei = y>HEIGHTPOS ? HEIGHTPOS : y;
		Bitmap *pFgbitmap = bmpGameWin.Clone(0, y - nHei, iWidth, nHei, PixelFormat32bppRGB);
		HGLOBAL hMemImage = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMemImage == NULL)
		{
			return -1;
		}
		IStream* pStmImage = NULL;
		CreateStreamOnHGlobal(hMemImage, TRUE, &pStmImage);
		if (pStmImage == NULL)
		{
			GlobalUnlock(hMemImage);
			GlobalFree(hMemImage);
			return -1;
		}
		pFgbitmap->Save(pStmImage, &clsid);

		LARGE_INTEGER liBegin = { 0 };
		pStmImage->Seek(liBegin, STREAM_SEEK_SET, NULL);

		BYTE* pbyImage = (BYTE *)GlobalLock(hMemImage);
		TransImage(pbyImage, GlobalSize(hMemImage), 0, y);//ＵＤＰ发送数据

		pStmImage->Release();
		GlobalUnlock(hMemImage);
		GlobalFree(hMemImage);
		delete pFgbitmap;
	}
	return 1;
}