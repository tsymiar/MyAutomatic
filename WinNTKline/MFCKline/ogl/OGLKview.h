#ifndef OGL_KVIEW_H_
#define OGL_KVIEW_H_
//#pragma execution_character_set("utf-8")
//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") 
#define GLEW_STATIC
#ifdef GLEW_STATIC 
#pragma comment(lib, "glew32s.lib") 
#else
#pragma comment(lib, "glew32.lib") 
#endif
#ifdef _MSC_VER
#pragma warning (disable:4067)
#pragma warning (disable:4477)
#pragma warning (disable:4996)
#endif
#define NO_WARN_MBCS_MFC_DEPRECATION
#define BUFFER_OFFSET(offset) ((void*)(offset))
#include	<cfloat>
#include	<cmath>
#include	<ctime>
#include	<list>
#include	<map>
#include	<vector>
#include	<iterator>
//#include <thread> #error <thread> is not supported when compiling with /clr or /clr:pure.
#include	<functional>
#include	<algorithm>
#include	<stdio.h>
#include	<io.h>
#include	<iostream>
#include	<fcntl.h>
#include	<fstream>
#include	<plus1second.h>
#ifdef _WIN32//__linux
#include "..\stdafx.h"
#include <Windows.h> 
#include <WinSock2.h>
#include <process.h>
#endif
#pragma comment(lib, "freetype.lib") 
#include	<font/ft2build.h>
#include	FT_FREETYPE_H  
#include	<GL/glew.h>  
#include	<GL/glut.h>
#include	<GL/freeglut.h> 
#include	<GL/GLU.h>
#include	<GL/GL.h>
#include	"boost/boostest.h"
#include	"Index/Indexes.inl"
#include	"Index/CPUID.H"
#include	"Index/_string.h"
#include	"dos/DOSCout.h"
#include	"Stock/Stock.h"
#include	"Def/MacroDef.h"
#include	"font/freetype/ftglyph.h" 
#ifdef _FILL_
#define fixpixelx 0.002f
#else
#define fixpixelx 0.002f
#endif
//#define GLTEST

class OGLKview
{
private:
	bool limitup;//harden涨停
	float radius;
	float moveDist;
	int item0;
	int pory, widt, heig;
	_string m_str;
public:
	OGLKview();
	virtual ~OGLKview();
public:
	typedef HWND(WINAPI *PROCGETCONSOLEWINDOW)();
	typedef char* SB;

	struct Market {
		tm time;
		float open;
		float close;
		float high;
		float low;
		int amount;
		float price;
		struct Market *next;
		bool operator >(const Market &toless)const
		{
			return high > toless.high;
		}
		bool operator <(const Market &togreater)const
		{
			return low < togreater.low;
		}
		void show();
	};
	struct Dlginfo {
		int line;
		int width;
		int multi;
		int height;
		int insert;
		int cycle = 222;
		int mouX;
		int mouY;
		int drawstaff;
		bool bkg = true;
		bool leftdown = false;
	};
	struct Fillitem {
		float ask1;
		float bid1;
		float curprice;
		float toopen;
		float upadp;
		float higest;
		float uprange;
		float lowest;
		int gross;
		float closing;
		float closed;
	};
	struct Charmarket {
		char	   low[16];
		char	  time[16];
		char	  open[16];
		char	  high[16];
		char	 close[16];
		char	 price[16];
		char	amount[16];
		char	  ydeg[64];
		char	 hintx[16];
		char		 y[64];
	};
	struct Item {
		struct time {
			int hour;
			int min;
		}time;
		float pc_;
		int mode;
		OGLKview::SB bs;
	};
	struct OglAttr {
		int wide;
		int tall;
	};
	struct FixWhat {
		int datacol = 0;
		int move = 0;
		float ratio = 1;
		float zoom = 1;
	};
	struct ViewSize {
		GLsizei ptx;
		GLsizei pty;
		GLsizei ptw; 
		GLsizei pth;
	};
	struct ZOOM
	{
		float z_vol = 1;
	};
	typedef struct Strmap {
		std::string code;
		std::string info;
	}Strmap;
	typedef Indexes::GLPoint Point;
	typedef Indexes::GLColor Color4f;
public:
	std::list<struct OGLKview::Market> OGLKview::Vec2List(std::vector<struct OGLKview::Market> marvec)
	{
		std::list<struct OGLKview::Market> marklist;
		std::copy_n(std::make_move_iterator(marvec.begin()), marvec.size(), std::back_inserter(marklist));
		marvec.clear();
		return marklist;
	}
	float OGLKview::SortPrice(std::list<struct OGLKview::Market> market, bool greater)
	{
		if (!greater)
		{
			market.sort(std::less<struct OGLKview::Market>());
			std::list<struct OGLKview::Market>::iterator it = market.begin();
			return it->high;
		}
		else
		{
			market.sort(std::greater<struct OGLKview::Market>());
			std::list<struct OGLKview::Market>::iterator it = market.begin();
			return it->low;
		}
	}
	std::vector<struct OGLKview::Market> OGLKview::SelectPeriod(std::vector<struct OGLKview::Market> market, unsigned space)
	{
		std::vector<struct OGLKview::Market>::iterator it = market.begin();
		if (market.size() > space)
		{
			it = market.begin() + space;
			market.erase(it, market.end());
			it = market.begin();
			return market;
		}
		else return market;
	}
private:
	HGLRC m_hRC;
	Point itempt;
	boostest buset;
	OGLKview* Okv;
	PROCGETCONSOLEWINDOW GetConsoleWindow;
	OGLKview::Point pt[2] = { { 1.7f,0 },{ -9.0,0 } };
	bool chart_frame(void);
	int diag_staff(int x, int y);
	void draw_string(const char* str);
	void GetChangeMatrix(float &angel, float &x, float &y, float &z);
public:
	bool unfurl;
	bool coding;
	float y_fix = 0;
	DOSCout DOS;
	ZOOM Zoom;
	Indexes index;
	FixWhat tinkep;
	Dlginfo dlginfo;
	Fillitem fillitem;
	OGLKview::Market lastmarket;
	std::map<int, OGLKview::Strmap>stockmap;
public:
	void _stdcall InitGraph(void);
	void SetBkg(bool b);
	void DrawItem(void);
	void SetColor(OGLKview::Color4f color);
	void DrawDash(OGLKview::Point pt[2]);
	void DrawCurve(OGLKview::Point A[4]);
	void DrawLevel(float mascl,float miscl);
	void AdjustDraw(GLsizei W, GLsizei H, bool b = true);
	void SwitchViewport(int viewport, OGLKview::ViewSize adjust = {1,1,1,1});
	void DrawKtext(char text[], Point &coor, int size = 14, OGLKview::Color4f color = {1,1,1,1}, char font[] = "Arial", bool dim=true);
	int DrawCoord(int mX, int mY);
	int DrawArrow(OGLKview::Point begin);
	int DrawDetail(OGLKview::Market market);
	int DrawPoly(OGLKview::Point Pb, OGLKview::Point Pe, OGLKview::Color4f color = {1,1,1}, int viewport = 1);
	int Data2View(std::vector<struct OGLKview::Market> market, OGLKview::Dlginfo toview);
	bool SetWindowPixelFormat(HDC m_hDC, HWND m_hWnd, int pixelformat = 0);
	bool DrawKline(OGLKview::Market markdata, OGLKview::FixWhat co, bool hollow = 1, OGLKview::Point pt = { 0 });
public:
	float axistinker(int pX)
	{
		return pX > 680 ? (float)(pX*fixpixelx - 1) : (float)(pX*fixpixely + 1);
	}
	float paramtinker(float param)
	{
		return 0.8f*(param*0.16f - 0.8f)*tinkep.move*tinkep.ratio;
	}
	OGLKview::Point xytinker(OGLKview::Point xy)
	{
		xy.x = (float)(xy.x*fixpixelx - 1.234f);
		xy.y = (float)(1.39f - xy.y*fixpixely);
		return xy;
	} 
	float OGLKview::Pxtinker(OGLKview::FixWhat tinker)
	{
		return (tinker.datacol - tinker.zoom)*tinker.ratio*0.01f-1.F;
	}
	float Pytinker(float py, OGLKview::FixWhat tinker)
	{
		return 0.27f*py*tinker.ratio*tinker.zoom + y_fix - 2.1f;
	}
};
#endif // !OGL_KVIEW_H_