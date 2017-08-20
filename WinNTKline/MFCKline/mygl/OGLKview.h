#if !defined(OGL_KVIEW_H_)
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
#pragma warning (disable:4067) // 预处理器指令后有意外标记 - 应输入换行符
#pragma warning (disable:4091) // “__declspec(dllexport)”: 没有声明变量时忽略“OGLKview”的左侧
#pragma warning (disable:4098) // 默认库“msvcrtd.lib”与其他库的使用冲突；请使用 /NODEFAULTLIB:library
#pragma warning (disable:4099) // 未找到 PDB“vc120.pdb”(使用“glew32s.lib(glew.obj)”或在“F:\dell - pc\Documents\GitHub\MyAutomatic\WinNTKline\Release\vc120.pdb”中寻找)；正在链接对象，如同没有调试信息一样
#pragma warning (disable:4244) // 从“double”转换到“float”，可能丢失数据
#pragma warning (disable:4248) // 无法解析 typeref 标记(01000011)(为“GLUtesselator”)；映像可能无法运行
#pragma warning (disable:4305) // “-=”: 从“double”到“GLfloat”截断
#pragma warning (disable:4477) // “sprintf_s”: 格式字符串“%s”需要类型“char *”的参数，但可变参数 1 拥有了类型“int”
#pragma warning (disable:4996) // error C4996: 'scanf': This function or variable may be unsafe. Consider using scanf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
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
#include	<cstdio>
#include	<iostream>
#include	<fstream>
#include	<random>
#include	<io.h>
#include	<fcntl.h>
#if 0
#include	<plus1second>
#endif
#include	<string>
#ifdef _WIN32//__linux
#include "..\stdafx.h"
#ifdef Error //ws2tcpip.h 'Error' redefined.
#undef Error
#endif
#if !defined(_WINDOWS_)
#include <Windows.h> 
#endif
#pragma comment(lib, "WS2_32.lib")
#include <WinSock2.h>
#include <process.h>
#endif
#pragma comment(lib, "freetype.lib") 
#include	<GL/glew.h>  
#include	<GL/glut.h>
#include	<GL/freeglut.h> 
#include	<GL/GLU.h>
#include	<GL/GL.h>
#include	<font/ft2build.h>
#include	"font/freetype/ftglyph.h"
#include	FT_FREETYPE_H  
#if !defined QT_DLL
#include	"GlModel.h"	// QFile error
#endif
#ifdef BOOST
#include	"boost/boostest.h"
#endif // BOOST
#include	"Idx/Initialise-inl.h"
#include	"Idx/CPUID.H"
#include	"Idx/_String-inl.h"
#if !defined(QT_VERSION)
#include	"dos/DOSCout.h"
#endif
#include	"Stock/Stock.h"
#include	"Def/MacroDef.h" 
#ifdef _FILL_
#define fixpixelx 0.002f
#else
#define fixpixelx 0.002f
#endif
//#define GLTEST
#define _N_ 10
#ifdef OGL_KVIEW_H_
#define DLL_KVIEW_API __declspec(dllexport)
#else
#define DLL_KVIEW_API __declspec(dllimport)
#endif

DLL_KVIEW_API class OGLKview
{
private:
	bool limitup;//harden涨停
	float radius;
	float moveDist;
	int item0;
	_String m_str;
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
		int drawstaff = 0;
		bool bkg = true;
		bool leftdown = false;
	};
	struct Fillitem {
		float ask1;
		float bid1;
		float curprice;
		float toopen;
		float upadp = .0f;
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
		int move = 70;
		float ratio = 1;
		float zoom = 1;
		int multi = 1;
	};
	struct ViewSize {
		GLsizei tx;
		GLsizei ty;
		GLsizei tw; 
		GLsizei th;
	};
	struct ZOOM
	{
		float z_vol = 1;
	};
	typedef struct Strmap {
		std::string code;
		std::string info;
	}Strmap;
	typedef Initialise::GLPoint Point;
	typedef Initialise::GLColor Color4f;
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
	HWND m_hDlg;
	Point itempt;
#ifdef BOOST
	boostest buset;
#endif
#if !defined QT_DLL
	GlModel model;
#endif
	OGLKview* Okv;
	std::vector<char*> markdata;
	Market st_stock;
	PROCGETCONSOLEWINDOW GetConsoleWindow;
	std::vector<struct OGLKview::Market > vec_market;
	OGLKview::Point pt[2] = { { 1.7f,0 },{ -9.0,0 } };
	bool chart_frame(void);
	int diag_staff(int x, int y);
	void print_string(const char* str);
	void GetChangeMatrix(float &angel, float &x, float &y, float &z) const;
public:
	bool unfurl;
	bool coding;
	float y_fix = 0;
#if !defined(QT_VERSION)
	//DOSCout DOS;
#endif
	ZOOM Zoom;
	Initialise index;
	FixWhat tinkep;
	Dlginfo dlginfo;
	Fillitem fillitem;
	const char* file = nullptr;
	int		failmsg = 0;
	OGLKview::Market lastmarket;
	std::map<int, OGLKview::Strmap>stockmap;
public:
	void _stdcall InitGraph(void);
	void DrawItem(void);
	void DrawDash(OGLKview::Point pt[2]);
	void DrawCurve(OGLKview::Point A[4]);
	void DrawLevel(float mascl,float miscl);
	void DrawKtext(char text[], Point &coor, int size = 14, OGLKview::Color4f color = { 1,1,1,1 }, char font[] = "Arial", bool dim = true);
	int DrawCoord(int mX, int mY);
	int DrawArrow(OGLKview::Point begin);
	int DrawDetail(OGLKview::Market market);
	int DrawPoly(OGLKview::Point Pb, OGLKview::Point Pe, OGLKview::Color4f color = {1,1,1}, int viewport = 1);
	bool DrawKline(OGLKview::Market markdata, OGLKview::FixWhat co, bool hollow = 1, OGLKview::Point pt = { 0 });
	void AdjustDraw(GLsizei W, GLsizei H, bool b = true);
	void SwitchViewport(int viewport, OGLKview::ViewSize adjust = {1,1,1,1});
	void SetBkg(bool b);
	void SetColor(OGLKview::Color4f color);
	int Data2View(std::vector<struct OGLKview::Market> market, OGLKview::Dlginfo toview);
	bool SetWindowPixelFormat(HDC m_hDC, HWND m_hWnd, int pixelformat = 0);
	bool GetMarkDatatoDraw(void* P = nullptr, char* title = NULL);
public:
	inline float axistinker(int pX) const
	{
		return pX > 680 ? (float)(pX*fixpixelx - 1) : (float)(pX*fixpixely + 1);
	}
	inline float paramtinker(float param) const
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

#undef DLL_KVIEW_API
#endif // !OGL_KVIEW_H_}

