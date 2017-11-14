#include "OGLKview.h"

#pragma unmanaged 

#ifdef _MSC_VER
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

OGLKview::OGLKview() { ;}

#ifdef __linux//||_UNIX
int OGLKview::myGL(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	glutInitWindowSize(1280, 750);
	glutInitWindowPosition(640, 375);

	glutCreateWindow("OGLKline");

	glutReshapeFunc(AdjustDraw);
	glutDisplayFunc(InitGraph);

	glutMainLoop();
	return(0);
}
#elif _MSC_VER
bool OGLKview::SetWindowPixelFormat(HDC m_hDC, HWND m_hWnd, int pixelformat)
{
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),      // pfd的大小
		1,                                  // 版本号
		PFD_DRAW_TO_WINDOW |                // 支持窗口
		PFD_SUPPORT_OPENGL |                // 支持OpenGL
		PFD_DOUBLEBUFFER,                   // 双缓存
		PFD_TYPE_RGBA,                      // RGBA类型
		24,                                 // 24位颜色深度
		0,0,0,0,0,0,                        // 忽略颜色位
		0,                                  // 无alpha缓存
		0,                                  // 忽略转换位
		0,                                  // 无累计缓存
		0,0,0,0,                            // 忽略累计位
		32,                                 // 32位深度缓存
		0,                                  // 无模版缓存
		0,                                  // 无辅助缓存
		PFD_MAIN_PLANE,                     // 主层（main layer）
		0,                                  // 保留（reserved）
		0,0,0,
	};
	if (!(pixelformat = ChoosePixelFormat(m_hDC, &pfd)))
	{
		::PostMessage(m_hWnd, WM_MSG_OGL, 0, (LPARAM)index.AllocBuffer(_T("Choose Pixel Format failed !")));
		return false;
	}
	if (!SetPixelFormat(m_hDC, pixelformat, &pfd))
	{
		::PostMessage(m_hWnd, WM_MSG_OGL, 1, (LPARAM)index.AllocBuffer(_T("Set Pixel Format failed !")));
		return false;
	}
	if (!(m_hRC = wglCreateContext(m_hDC)))
	{
		::PostMessage(m_hWnd, WM_MSG_OGL, 2, (LPARAM)index.AllocBuffer(_T("CreateContext failed!")));
		return false;
	}
	if (!wglMakeCurrent(m_hDC, m_hRC))
	{
		::PostMessage(m_hWnd, WM_MSG_OGL, 3, (LPARAM)index.AllocBuffer(_T("MakeCurrent failed!")));
		return false;
	}
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32"));
	GetConsoleWindow = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, ("GetConsoleWindow"));
	this->m_hDlg = m_hWnd;
	return true;
}
//unsigned _stdcall iItem(void* p)
//{
//	OGLKview* kv = (OGLKview*)p;
//	TH *th = (TH*)p;
//}
#endif

using namespace std;

bool g_multi = false;
char g_mktim[16] = { NULL };
char g_ltime[32] = { NULL };
float Xdeg, Ydeg;
float diff = 0, voly = 0;
float proportion, delta = 1;
float shadowup, shadowdown;
float candlemiddle, candleright, candleleft;
OGLKview::Point g_P, arrow;
OGLKview::Market fixeddata;
OGLKview::OglAttr attr = { 1366,768 };
OGLKview::Item g_ITEM = { 0,0,0,NULL };
OGLKview::ViewSize viewsize = { 0,0,0,0 };
OGLKview::Charmarket markchars = { "---","---","---" };
#ifdef __linux
#define _Strtok strtok_r
#elif _MSC_VER
#define _Strtok strtok_s
#endif

bool OGLKview::DrawKline(OGLKview::Market markdata, OGLKview::FixWhat co, bool hollow, OGLKview::Point pt)
{
	//(HANDLE)_beginthreadex(NULL, 0, &iItem, &thd, 0, NULL);
	SwitchViewport(1);
#if _DEBUG
	glPointSize(3);
	glBegin(GL_POINTS);
	glVertex3f(0, 0, 0);
	glEnd();
#endif
	sprintf_s(g_ltime, "%d-%d-%d", markdata.time.tm_year, markdata.time.tm_mon, markdata.time.tm_mday);
	tinkep.datacol = dlginfo.line;// -3;
	hollow = !dlginfo.bkg;
	g_P.x = (float)dlginfo.mouX;
	g_P.y = (float)dlginfo.mouY;
	candlemiddle = Xdeg = pt.x = Okv->Pxtinker(co);
	fillitem.closing = markdata.close;
	fillitem.curprice = markdata.price;
	fillitem.gross = markdata.amount;
	fillitem.toopen = markdata.open;
	fillitem.upadp = markdata.close - markdata.open;
	fillitem.closed = lastmarket.close;
	markdata.open != 0 ? \
		(fillitem.uprange = fillitem.upadp / markdata.open) : 0;
	sprintf(markchars.close, "%.2f", markdata.close);
	sprintf(markchars.price, "%.2f", markdata.price);
	sprintf(markchars.amount, "%.2f", markdata.amount);
	sprintf(markchars.open, "%.2f", markdata.open);
	sprintf(markchars.high, "%.2f", markdata.high);
	sprintf(markchars.low, "%.2f", markdata.low);
	if (fillitem.higest < markdata.high)
		fillitem.higest = markdata.high;
	if (fillitem.lowest < markdata.low)
		fillitem.lowest = markdata.low;
	//
	if (tinkep.ratio < 1)
		g_multi = true;
	else
		g_multi = false;
	fixeddata.open = Pytinker(markdata.open, tinkep);
	fixeddata.close = Pytinker(markdata.close, tinkep);
	fixeddata.high = Pytinker(markdata.high, tinkep);
	fixeddata.low = Pytinker(markdata.low, tinkep);
	if (fillitem.upadp - 0.f > 0)
	{
		limitup = true;
		glColor3f(1, 0, 0);
		shadowup = fixeddata.close;
		shadowdown = fixeddata.open;
	}
	else if (fillitem.upadp - 0.f < 0)
	{
		limitup = false;
		if (dlginfo.bkg)
			glColor3f(0, 1, 1);
		else
			glColor3f(0.1f, 0.7f, 0.3f);
		shadowup = fixeddata.open;
		shadowdown = fixeddata.close;
	}
	else {
		glColor3f(1, 1, 1);
		shadowup = shadowdown = \
			fixeddata.open = fixeddata.close;
	}
	//边界
	candleleft = candlemiddle - 0.0037f*tinkep.ratio;
	candleright = candlemiddle + 0.0037f*tinkep.ratio;
	GLfloat quad[] = {
		candleright,shadowup,
		candleleft,	shadowup,
		candleleft,	shadowdown,
		candleright,shadowdown
	};
	if (fixeddata.high != fixeddata.low)
		proportion = delta / (fixeddata.high - fixeddata.low);
	else {};
	if (fabs(fillitem.upadp) < 0.01f)
		fixeddata.open -= 0.009f;
	//比例超过临界
	if (g_multi)
	{
		candlemiddle /= 2;
		candleleft = candleright = candlemiddle;
		glBegin(GL_LINE);
		glVertex2f(candlemiddle, shadowup);
		glVertex2f(candlemiddle, shadowdown);
		glEnd();
	}
	//阴烛
	else
		if (!limitup)
		{
			glInterleavedArrays(GL_V2F, 0, quad);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	//阳烛
		else
			if (!hollow)
			{
				GLfloat holldata[] = {
					candleleft,	shadowup,
					candleright,shadowup,
					candleleft,	shadowdown,
					candleleft,	shadowup,
					candleright,shadowup,
					candleright,shadowdown,
					candleright,shadowdown,
					candleleft,	shadowdown
				};
				glInterleavedArrays(GL_V2F, 0, holldata);
				glDrawArrays(GL_LINES, 0, 8);
#if !defined(CMfcKView)
				//assert(_CrtCheckMemory());
#endif
			}
			else
			{
				glInterleavedArrays(GL_V2F, 0, quad);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}
	limitup ? shadowup = float(fixeddata.close - fixpixely) : \
		shadowup = float(fixeddata.open - fixpixely);
	glBegin(GL_LINES);
	{	//上影
		glVertex2f(candlemiddle, fixeddata.high);
		glVertex2f(candlemiddle, shadowup);
		//下影
		glVertex2f(candlemiddle, shadowdown);
		glVertex2f(candlemiddle, fixeddata.low);
	}
	glEnd();
	Ydeg = (fixeddata.high + fixeddata.low) / 2;
	if ((xytinker(g_P).x >= candleleft) && (xytinker(g_P).x <= candleright))
	{
		glColor3f(0.902f, 0.902f, 0.980f);
		glPointSize(3);
		glDisable(GL_DEPTH_TEST);
		glBegin(GL_POINTS);
		glVertex2f(candlemiddle, Pytinker(markdata.close, tinkep));
		glEnd();
		glEnable(GL_DEPTH_TEST);
		memcpy(g_mktim, markchars.time, strlen(markchars.time));
		g_mktim[strlen(g_mktim)] = '\0';
		sprintf(markchars.time, "%d.%d.%d", markdata.time.tm_year, markdata.time.tm_mon, markdata.time.tm_mday);
		sprintf(markchars.amount, "%d", markdata.amount);
		sprintf(markchars.price, "%.0f", markdata.price);
		sprintf(markchars.close, "%.2f", markdata.close);
		sprintf(markchars.high, "%.2f", markdata.high);
		sprintf(markchars.low, "%.2f", markdata.low);
		sprintf(markchars.open, "%.2f", markdata.open);
		sprintf(markchars.ydeg, "y=%.3f", Ydeg);
	}
	if ((xytinker(g_P).x >= fixeddata.low) && (xytinker(g_P).x <= fixeddata.high))
	{
		sprintf(markchars.hintx, "%.2f", markdata.close);
		sprintf(markchars.y, "K(y)=%.2f", xytinker(g_P));
	}
	arrow.y = Pytinker(fillitem.higest, tinkep);
	if (1/*orighigh>=p_maxpri*/)
	{
		arrow.y = fixeddata.high;
	}
	else { ; }
	//红蜡烛黑色背景
	if (limitup && dlginfo.bkg)
	{
		glColor3f(0, 0, 0);
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.f, 0.f);
			glVertex2f(candleright, fixeddata.low);
			glTexCoord2f(1.f, 0.f);
			glVertex2f(candleleft, fixeddata.low);
			glTexCoord2f(1.f, 1.f);
			glVertex2f(candleleft, float(fixeddata.high + fixpixely));
			glTexCoord2f(0.f, 1.f);
			glVertex2f(candleright, float(fixeddata.high + fixpixely));
		}
		glEnd();
	}
	//成交量Vol
	SwitchViewport(2);
	diff = fillitem.closing - fillitem.closed;
	(diff - 0.f > 0) ? glColor3f(1, 0, 0) : \
		glColor3f(0, 1, 1);
	voly = (markdata.amount*pow(10.0f, -8) - 0.7f)*Zoom.z_vol;
	if (g_multi)
	{
		glBegin(GL_LINES);
		glVertex2f(candlemiddle, voly);
		glVertex2f(candlemiddle, -10.0f);
		glEnd();
		candleleft = candleright = candlemiddle;
	}
	if ((diff > 0) && dlginfo.bkg)
	{
		glBegin(GL_LINES);
		glVertex2f(candleleft, -10);
		glVertex2f(candleleft, voly);
		glVertex2f(candleleft, voly);
		glVertex2f(candleright, voly);
		glVertex2f(candleright, voly);
		glVertex2f(candleright, -10);
		glEnd();
	}
	else {
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.f, 0.f);
			glVertex2f(candleright, -10);
			glTexCoord2f(1.f, 0.f);
			glVertex2f(candleleft, -10);
			glTexCoord2f(1.f, 1.f);
			glVertex2f(candleleft, voly);
			glTexCoord2f(0.f, 1.f);
			glVertex2f(candleright, voly);
		}
		glEnd();
	};
	fillitem.closed = fillitem.closing;
	return hollow;
}

void _stdcall OGLKview::InitGraph(void/*HDC m_hDC*/)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDepthMask(GLU_TRUE);
	glShadeModel(GLU_FLAT);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -3.0f);
	glViewport(0, 0, attr.wide, attr.tall);
#ifdef DEBUG
	glPointSize(3);
	glColor3f(0.9f, .4f, 0);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH, GL_NICEST);
	glBegin(GL_POINTS);
	{
		glVertex3f(0, 0, 0);
	}
	glEnd();
#endif // DEBUG 
#if !defined(GLTEST) 
#ifdef __linux
	OGLKview ogl;
	ogl.
#endif
		chart_frame();
#ifdef __linux
	ogl.diag_staff(ogl.dlginfo.mouX, ogl.dlginfo.mouY);
#else
	this->diag_staff(dlginfo.mouX, dlginfo.mouY);
#endif
#if defined BOOST
	buset.m_boostest();
#endif
#else
#if !defined(QT_VERSION)
	model.Model(attr.wide, attr.tall, dlginfo.mouX, dlginfo.mouY);
#endif
#endif
#ifdef _CONSOLE||_WINDOWS
	SetConsoleCtrlHandler(dos.ConsoleHandler, TRUE);
#endif
	//SwapBuffers(m_hDC);
}

#ifdef _WIN32
void OGLKview::print_string(const char* str)
{
	int len = 0, i;
	static wchar_t* wstring = NULL;
	HDC m_hDC = wglGetCurrentDC();
#ifdef _BYTE
	GLuint list = glGenLists(128);
	for (; str[len] != '\0'; ++len)
	{
		if (IsDBCSLeadByte(str[len]))
			++len;
	}
	len += 1;
	//wstring = new wchar_t(len);
	if ((wstring = (wchar_t*)malloc(len * sizeof(wchar_t))) == NULL)
		return;
#else
	GLuint list = glGenLists(1);
	for (i = 0; str[i] != '\0'; ++i)
	{
		if (IsDBCSLeadByte(str[i]))
			++i;
		++len;
	}
#if !defined(_UNICODE)
#define _UNICODE
	wstring = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
#undef _UNICODE
#else
	wstring = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
#endif
#endif
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wstring, len);
	wstring[len] = _T('\0');
	if ((wstring[0] > 0x7f) || (47 < wstring[0] && wstring[0] <= 127))
		//if (wstring[0] > 127)
		i = 0;
	else
		i = -1;//ASCII
	for (; i < len; ++i)
	{
#if !defined(_UNICODE)
#define _UNICODE
		if (m_hDC != NULL)
			wglUseFontBitmapsW(m_hDC, wstring[i], 1, list);
#undef _UNICODE
#endif
		glCallList(list);
	}
	if (wstring != NULL)
	{
		free(wstring);
		wstring = NULL;
	}
	glDeleteLists(list, 1);
}
#endif

int OGLKview::diag_staff(int x, int y)
{
	OGLKview::Point pt = \
	{ pt.x = (float)x, pt.y = (float)y };
	glViewport(0, viewsize.ty, viewsize.tw, viewsize.th);
	glColor3f(1, 1, 1);
	glBegin(GL_LINES);
	{
		glVertex2f(-10.0f, this->xytinker(pt).y);
		glVertex2f(10.0f, this->xytinker(pt).y);
	}
	glEnd();
	glViewport(0, 0, viewsize.tw, int(viewsize.th*1.966f));
	glBegin(GL_LINES);
	{
		glVertex2f(this->xytinker(pt).x, -1.239f);
		glVertex2f(this->xytinker(pt).x, 1.17f);
	}
	glEnd();
	return y;
}

bool ChartItem(int row, OGLKview::Market market)
{
	char info[32];
	char listime[32];
	OGLKview view;
	OGLKview::Point pt;
	bool tmp = *((bool*)&view);
	//GLuint index = 1;
	//glNewList(index, GL_COMPILE);
	g_ITEM.pc_ = market.price;
	glViewport(0, 100, attr.wide, attr.tall - 100);
	pt.x = 0.73f;
	pt.y = 0.6f - 0.08f;
	//if (pt.y <= -0.9f)pt.y = 0.6f;
	g_ITEM.mode = 1;
	sprintf_s(listime, "%s", g_mktim);
	view.DrawKtext(listime, pt, 13);
	pt.x = 0.9f;
	sprintf_s(info, "%.2f", g_ITEM.pc_);
	view.DrawKtext(info, pt, 13);
	pt.x = 1.13f;
	_itoa_s(g_ITEM.mode, info, 10);
	view.DrawKtext(info, pt, 13);
	pt.x = 1.17f;
	if (tmp)
	{
		g_ITEM.bs = "B";
		glColor3f(1, 0, 0);
	}
	else
	{
		g_ITEM.bs = "S";
		glColor3f(0, 1, 0);
	}
	view.DrawKtext(g_ITEM.bs, pt, 13);
	return tmp;
}

void OGLKview::AdjustDraw(GLsizei W, GLsizei H)
{
	attr.wide = W;
	attr.tall = H;
	glViewport(0, 0, W, H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (H != 0)
		gluPerspective(45.0f, W / H, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
}

int OGLKview::DrawCoord(int mX, int mY)
{
	int	msgid[4];
	char coor[16];
	char brand[64];
	OGLKview::Point pt = { 0,1.19f };
	glViewport(0, 0, attr.wide, attr.tall);
	int m0 = mX / 1000;
	int m1 = mX / 100 % 10;
	int m2 = mX / 10 % 10;
	int m3 = mX % 10;
	int m4 = mY / 1000;
	int m5 = mY / 100 % 10;
	int m6 = mY / 10 % 10;
	int m7 = mY % 10;
	coor[0] = '(';
	coor[1] = 'x';
	coor[2] = '=';
	coor[3] = m0 + 48;
	coor[4] = m1 + 48;
	coor[5] = m2 + 48;
	coor[6] = m3 + 48;
	coor[7] = ',';
	coor[8] = 'y';
	coor[9] = '=';
	coor[10] = m4 + 48;
	coor[11] = m5 + 48;
	coor[12] = m6 + 48;
	coor[13] = m7 + 48;
	coor[14] = ')';
	coor[15] = '\0';
	DrawKtext(coor, pt, 14, { 1,1,0 });
#ifdef _MSC_VER
	char cpuattr[64];
	cpu_getbrand(brand);
	__cpuidex(msgid, 0, 0);
	sprintf_s(cpuattr, "CPU0:%d-%s", msgid[1], brand);
	pt = { -0.9f,1.19f };
	DrawKtext(cpuattr, pt, 14, { 1,1,0 });
#endif
	return m7;
}


int OGLKview::DrawArrow(OGLKview::Point begin)
{
	enum VAO_IDs { Triangles, NumVAOs };
	enum Buffer_IDs { ArrayBuffer, NumBuffers };
	enum Attrib_IDs { vPosition = 0 };
	OGLKview::Point end, pt[2];

	glColor3f(1, 1, 1);
	end.x = begin.x - 0.019f*tinkep.ratio;
	end.y = begin.y;
	pt[0] = begin;
	pt[1] = end;

	const GLfloat vertices[] = {
		begin.x,begin.y,//0.0f,1.0f,
		(begin.x - 0.003f*tinkep.ratio) * 2 / (float)sqrt(3),begin.y + 0.001f*tinkep.ratio,//0.0f,1.0f,
		(begin.x - 0.003f*tinkep.ratio) * 2 / (float)sqrt(3),begin.y - 0.001f*tinkep.ratio,//0.0f,1.0f,
	};
	const GLuint NumVertices = sizeof(vertices) / sizeof(GLfloat);

#ifdef _glew_h_  
	GLuint  VAOs[NumVAOs];
	GLuint  Buffers[NumBuffers];
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();

	if (glewError != GLEW_OK)
	{
		exit(EXIT_FAILURE);
	}
	glGenVertexArrays(NumVAOs, VAOs);
	glBindVertexArray(VAOs[Triangles]);
	glGenBuffers(NumBuffers, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[ArrayBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vPosition);

	glBindVertexArray(VAOs[Triangles]);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	//glFlush();

	glBegin(GL_LINES);
	glVertex2f(begin.x, begin.y);
	glVertex2f(end.x, end.y);
	glEnd();
#endif
	return NumVertices;
}

int OGLKview::DrawDetail(OGLKview::Market market)
{
	float x = 0, y = 0;
	static char hintx[16];
	float min = 0, step = 1, fold = 1;
	static OGLKview::Point txcr, mc;
	static Color4f f4;
	txcr.x = -1.234567f;
	txcr.y = 1.17f;
	mc.x = (float)dlginfo.mouX;
	mc.y = (float)dlginfo.mouY;
	min += (417 - dlginfo.mouY)*(float)fixpixely*step / fold*3.9f;
	sprintf(hintx, "%.2f", min);
	SwitchViewport(1);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	//↗glVertex3f(x,y,z);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(-1.03f, 0.333f, 0);
	//↘
	glTexCoord2f(1.f, 0.f);
	glVertex3f(-1.3f, 0.333f, 0);
	//↙丄
	glTexCoord2f(0.f, 0.f);
	glVertex3f(-1.3f, 1.2417f, 0);
	//↖
	glTexCoord2f(0.f, 1.f);
	glVertex3f(-1.03f, 1.2417f, 0);
	glEnd();

	glColor3f(1, 1, 1);
	glBegin(GL_LINES);
	glVertex3f(-1.3f, 0.333f, 0);
	glVertex3f(-1.03f, 0.333f, 0);

	glVertex3f(-1.3f, 0.333f, 0);
	glVertex3f(-1.3f, 1.2417f, 0);

	glVertex3f(-1.3f, 1.2417f, 0);
	glVertex3f(-1.03f, 1.2417f, 0);

	glVertex3f(-1.03f, 1.2417f, 0);
	glVertex3f(-1.03f, 0.333f, 0);
	glEnd();

	DrawKtext(g_ltime, txcr, 17, { 0.6f, 0.7f, 0.8f, 0.9f }, "微软雅黑");
	txcr.y = 1.09f;
	DrawKtext("开盘", txcr, 13, { 1,1,1 }, "宋体");
	txcr.y = 0.9f;
	DrawKtext("最高", txcr, 13, { 1,1,1 }, "宋体");
	txcr.y = 0.7f;
	DrawKtext("最低", txcr, 13, { 1,1,1 }, "宋体");
	txcr.y = 0.5f;
	DrawKtext("收盘", txcr, 13, { 1,1,1 }, "宋体");
	//方框
	//if((fixxy(iop).x>marketraw.left)&&(fixxy(iop).x>marketraw.right))
	//{
	txcr.y = 1.0f;
	if (limitup)
		f4 = { 1,0,0 };
	else
		f4 = { 0,1,0 };
	DrawKtext(markchars.open, txcr, 14, f4, "微软雅黑");
	txcr.y = 0.8f;
	DrawKtext(markchars.high, txcr, 14, f4, "微软雅黑");
	txcr.y = 0.6f;
	DrawKtext(markchars.low, txcr, 14, f4, "微软雅黑");
	txcr.y = 0.4f;
	DrawKtext(markchars.close, txcr, 14, f4, "微软雅黑");
	//}
	glEnable(GL_DEPTH_TEST);
	//竖轴提示框
	SwitchViewport(0);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0, .8f, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(xytinker(txcr).x, -1.2f, 0);
	glTexCoord2f(1, 0);
	glVertex3f(xytinker(txcr).x, -1.239f, 0);
	glTexCoord2f(1, 1);
	if (xytinker(txcr).x + .1f <= -1.07f)
		x = xytinker(txcr).x + 0.19f;
	else
		x = xytinker(txcr).x - 0.19f;
	glVertex3f(x, -1.239f, 0);
	glTexCoord2f(0, 1);
	glVertex3f(x, -1.2f, 0);
	glEnd();
	glColor3f(1, 1, 1);
	glBegin(GL_LINES);
	glVertex3f(xytinker(txcr).x, -1.2f, 0);
	glVertex3f(xytinker(txcr).x, -1.239f, 0);
	glVertex3f(x, -1.2f, 0);
	glVertex3f(x, -1.239f, 0);
	glVertex3f(x, -1.2f, 0);
	glVertex3f(xytinker(txcr).x, -1.2f, 0);
	glVertex3f(x, -1.239f, 0);
	glVertex3f(xytinker(txcr).x, -1.239f, 0);
	glEnd();
	txcr.y = 1.23f;
	if (xytinker(txcr).x + .1f <= -1.07f)
		txcr.x = x - 1.07f;
	else
		txcr.x = x + .01f;
	DrawKtext(g_ltime, txcr, 15, { 1,1,0,1 }, "宋体");
	//横轴
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(1.1f, xytinker(txcr).y, 0);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(1.3f, xytinker(txcr).y, 0);
	glTexCoord2f(1.f, 1.f);
	if (xytinker(txcr).y + .1f >= 1.2417f)
		y = xytinker(txcr).y - .1f;
	else
		y = xytinker(txcr).y + .1f;
	glVertex3f(1.3f, y, 0);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(1.1f, y, 0);
	glEnd();
	glColor3f(1, 1, 1);
	glBegin(GL_LINES);
	glVertex3f(1.2409f, xytinker(txcr).y, 0);
	glVertex3f(1.1f, xytinker(txcr).y, 0);
	glVertex3f(1.2409f, xytinker(txcr).y, 0);
	glVertex3f(1.2409f, y, 0);
	glVertex3f(1.2409f, y, 0);
	glVertex3f(1.1f, y, 0);
	glVertex3f(1.1f, y, 0);
	glVertex3f(1.1f, xytinker(txcr).y, 0);
	glEnd();
	txcr.x = 1.13f;
	if (xytinker(txcr).y + .1f >= 1.2417f)
		txcr.y = y + .02f;
	else
		txcr.y = y - .08f;
	DrawKtext(hintx, txcr, 15, { 1, 1, 0, 1 }, "宋体");
	glEnable(GL_DEPTH_TEST);
	return (int)y;
}

int OGLKview::DrawPoly(OGLKview::Point Pb, OGLKview::Point Pe, OGLKview::Color4f color, int viewport)
{
	SwitchViewport(viewport);
	Pb.y = Pytinker(Pb.y, tinkep);
	Pe.y = Pytinker(Pe.y, tinkep);
	glColor3f(color.R, color.G, color.B);
	glBegin(GL_LINES);
	glVertex2f(Pb.x, Pb.y);
	glVertex2f(Pe.x, Pe.y);
	glEnd();
	return viewport;
}

int OGLKview::Data2View(std::vector<struct OGLKview::Market> market, OGLKview::Dlginfo toview)
{
	//添加迭代器用于遍历向量元素
	std::vector<OGLKview::Market>::iterator it = market.begin();
	while (it != market.end())++it;
	//std::vector<OGLKview::Market>::iterator element = std::find(market.begin(), market.end(), who);
	//if (element != market.end())int pos = std::distance(market.begin(), element);
	OGLKview::Point fp;
	fp.x = 974.f - toview.mouX;
	fp.y = 0;
	int crkp = int(toview.line - xytinker(fp).x / (0.02f*toview.multi));
	if ((market.size() > (unsigned)crkp) && (crkp >= 0)) {
	}
	return 0;
}

void OGLKview::SetBkg(bool b)
{
	if (b)
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	else
		glClearColor(0.8f, 0.9f, 0.7f, 0.9f);
}

void OGLKview::SetColor(OGLKview::Color4f color)
{
	if (color.A == 0)
		color.A = 1;
	glColor4f(color.R, color.G, color.B, color.A);
}

void OGLKview::SwitchViewport(int viewport, OGLKview::ViewSize adjust)
{
	switch (viewport)
	{
	case 0://全局
		glViewport(0, 0, attr.wide*adjust.tw, adjust.th*attr.tall);
		break;
	case 1://左上
		glViewport(0, int(attr.tall*(1 - 0.539f)), int(attr.wide*0.742f), int(attr.tall / 1.966f));
		break;
	case 2:
		glViewport(0, int(attr.tall*0.342f), int(attr.wide*0.742f), int(attr.tall*0.11f));
		break;
	case 3:
		glViewport(0, int(attr.tall*0.22f), int(attr.wide*0.72f), int(attr.tall*0.11f));
		break;
	case 4:
		glViewport(0, int(attr.tall*0.1f), int(attr.wide*0.72f), int(attr.tall*0.11f));
		break;
	default:	break;
	}
}

bool OGLKview::chart_frame()
{
	OGLKview::Point trade = { 0.72f,1.19f };
	glViewport(0, 0, attr.wide, attr.tall);
	if (!coding)
		DrawKtext("Strock code", trade, 17, { 1,1,0 }, "", false);
	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	{
		//x-axis
		//上边界
		glVertex2f(-9.0f, 1.2417f);
		glVertex2f(9.0f, 1.2417f);

		glVertex2f(0.7f, 1.17f);
		glVertex2f(1.7f, 1.17f);

		glVertex2f(0.6f, 1.17f);
		glVertex2f(-9.0f, 1.17f);

		glVertex2f(0.7f, -0.1f);
		glVertex2f(-9.0f, -0.1f);

		glVertex2f(0.7f, -0.4f);
		glVertex2f(-9.0f, -0.4f);

		glVertex2f(0.7f, -0.7f);
		glVertex2f(-9.0f, -0.7f);

		glVertex2f(0.7f, -1.0f);
		glVertex2f(-9.0f, -1.0f);

		glVertex2f(0.7f, -1.1f);
		glVertex2f(-9.0f, -1.1f);

		glVertex2f(1.7f, -1.17f);
		glVertex2f(-9.0f, -1.17f);
		//y-axis
		glVertex2f(1.7f, -1.17f);
		glVertex2f(1.7f, 1.2417f);

		glVertex2f(0.7f, -1.17f);
		glVertex2f(0.7f, 1.2417f);

		glVertex2f(0.6f, -1.17f);
		glVertex2f(0.6f, 1.2417f);
	}
	glEnd();
	return true;
}

void OGLKview::DrawDash(Point pt[2])
{
	glColor3f(1, 0, 0);
	glEnable(GL_LINE_STIPPLE);
	glLineWidth(0.5);
	glLineStipple(1, 0x4444);
	glBegin(GL_LINES);
	glVertex2f(pt[0].x, pt[0].y);
	glVertex2f(pt[1].x, pt[1].y);
	glEnd();
	glDisable(GL_LINE_STIPPLE);
}

void OGLKview::DrawCurve(OGLKview::Point A[4])
{
	Initialise idx;
	for (int i = 0; i <= 3; i++)
		A[i] = xytinker(A[i]);
	glColor3f(1, 1, 1);
	Point Pold = { A[0].x,A[0].y };
	for (double t = 0.f; t <= 1.f; t += .1f)
	{
		Point P = idx.CubicBezier/*CubicBézier*/(A, t);
		glBegin(GL_LINES);
		glVertex2f(Pold.x, Pold.y);
		glVertex2f(P.x, P.y);
		glEnd();
		Pold = P;
	}
}

void OGLKview::DrawLevel(float mascl, float miscl)
{
	char c_cl[32];
	float i = -0.1f;
	OGLKview::Point toe;
	float div = (mascl - miscl) / (8.0f*tinkep.ratio);
	SwitchViewport(0);
	for (; i < 1.17f; i += 0.16f) {
		if (miscl >= mascl)
			break;
		toe.x = 0.6099f;
		toe.y = i + 0.07f;
		sprintf_s(c_cl, "%.2f", miscl);
		DrawKtext(c_cl, toe, 15, { 1,0,1 });
		miscl += div;
	}
	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	for (i = -0.1f; i < 1.17f; i += 0.16f)
	{
		glVertex2f(0.6f, i + 0.08f);
		glVertex2f(0.58f, i + 0.08f);
		glVertex2f(0.6f, i);
		glVertex2f(0.59f, i);
	}
	glEnd();
}

void OGLKview::DrawItem()
{
	itempt.x = 0.8f;
	itempt.y = 0.007f*item0;
	if (itempt.y < -1)itempt.y = 0.7f;
	char listitem[256];
	sprintf(listitem, "%s\t%f\t%d", g_mktim, fillitem.closed, 1);
	DrawKtext(listitem, itempt, 15);
	item0++;
}

int OGLKview::DrawKtext(char text[], Point & coor, int size, OGLKview::Color4f color, char font[], bool dim)
{
#ifdef _WIN32
	int fw;
	if (dim)
		fw = FW_MEDIUM;
	else
		fw = FW_SEMIBOLD;
#if !defined(_UNICODE)
#define _UNICODE
#endif
	HFONT mhfont = CreateFont(size, 0, 0, 0, fw, 0, 0, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS,
#undef _UNICODE
#ifdef _UNICODE
		(const wchar_t *)
#endif
		font
	);
	HFONT hOldFont = (HFONT)SelectObject(wglGetCurrentDC(), mhfont);
	DeleteObject(hOldFont);
	if (color.A == 0)
		color.A = 1;
	glColor4f(color.R, color.G, color.B, color.A);
	glRasterPos2f(coor.x, coor.y);
	print_string(text);
#elif __linux 
	GLfloat texcoord[4];
	GLfloat texMinX, texMinY;
	GLfloat texMaxX, texMaxY;
	SDL_Color textColor = { 256 * color.R, 256 * color.G, 256 * color.B };
	SDL_Surface *message = NULL;
	SDL_GL_init();
	if (ttffont == NULL)
		ttffont = TTF_OpenFont("../MFCKline/font/simfang.ttf", size);
	if (ttffont == NULL)
	{
		fprintf(stderr, "font open failure %s\n", SDL_GetError());
		return -1;
	}
	if (dim)
		TTF_SetFontStyle(ttffont, TTF_STYLE_STRIKETHROUGH);
	else
		TTF_SetFontStyle(ttffont, TTF_STYLE_NORMAL);
	if (text[0] > 127)
		message = TTF_RenderUTF8_Solid(ttffont, text, textColor);//hanzi
	else
		message = TTF_RenderText_Solid(ttffont, text, textColor);//ascii         
	GLuint texture = SDL_GL_LoadTexture(message, texcoord);
	/* Make texture coordinates easy to understand */
	texMinX = texcoord[0];
	texMinY = texcoord[1];
	texMaxX = texcoord[2];
	texMaxY = texcoord[3];
	/* Show the text */
	SDL_GL_Enter2DMode(attr.wide, attr.tall);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(texMinX, texMinY); glVertex2i(50 * coor.x, 50 * coor.y);
	glTexCoord2f(texMaxX, texMinY); glVertex2i((50 + message->w)*coor.x, 50 * coor.y);
	glTexCoord2f(texMinX, texMaxY); glVertex2i(50 * coor.x, (50 + message->h)*coor.y);
	glTexCoord2f(texMaxX, texMaxY); glVertex2i((50 + message->w)*coor.x, (50 + message->h)*coor.y);
	glEnd();
	SDL_GL_Leave2DMode();

	SDL_GL_SwapBuffers();
	SDL_FreeSurface(message);
#endif
	return 0;
}
///**************GetMarkDatatoDraw()**************///
namespace GMDD
{
	int		i = 0;
	int		li = 0;//绘制曲线时每组下标
	int		line = 0;//当前读取行数
	int		idx = 0;
	int		item0 = 0;
	int		volume;//成交量
	float	maxprice = 0.0f;
	float	minprice = FLT_MAX;
	float	fo = 0;
	char*	cot = NULL;//切分临时数据
	char*	buff = NULL;
	char*	token = NULL;
	bool	isnext = false;//是否为下一组
	bool	hadraw5 = false;
	bool	hadraw10 = false;
	bool	hadraw20 = false;
	char*	div_stock[3] = { NULL };
	char	ma[32] = { NULL };
	char	dif[32] = { NULL };
	char	dea[32] = { NULL };
	char	rsi[32] = { NULL };
	char	vol[32] = { NULL };
	char	macd[32] = { NULL };
	char	code[64] = { NULL };
	char	s_code[] = "股票代码：";
	char	s_ma5[] = "SMA5:";
	char	s_ma10[] = "SMA10:";
	char	s_ma20[] = "SMA20:";
	char	s_dif[] = "DIF:";
	char	s_dea[] = "DEA:";
	char	s_rsi[] = "RSI(6,12,24):";
	char	s_vol[] = "成交量 Vol:";
	char	s_macd[] = "MACD(12,26,9)";
	Stock stock;
	std::vector<Stock::Rsi> strsi;
	std::string tmp = " ";
	std::ifstream Readfile;
#if 0
	std::ostream;
#endif
	std::string sWrite;
	const char* writefile;
	Stock::Rsi total{ 0 },/*分组变量，前一组最后一日收盘价*/last{ 0 },/*N日收盘涨幅和*/frise{ 0 },/*N日收盘跌幅和（绝对值）*/fdrop{ 0 };
	Stock::Sma::MA isma{ 0 },/*今日*/totma{ 0 }, /*昨日*/yma{ 0 };
	Stock::Sma ma20{ 0 }, ma10{ 0 }, ma5{ 0 };//MA均线
	OGLKview::Point Pter = { 0 },/*折线的前一个点*/ Pt[4] = { 0 };
	//一般展示均线
	OGLKview::Point pt_vol = { 0 }, pt_dif = { 0 }, pt_dea = { 0 }, pt_rsi = { 0 }, pt_macd = { 0 };
	//图例文字坐标
	OGLKview::Point pt_code = { 0.8f,1.189f }, pt_stock = { -1.22f,pt_code.y };
	//计算后的RSA
	OGLKview::Point pt_rsi6 = { 0 }, pt_rsi12 = { 0 }, pt_rsi24 = { 0 };
	//MA移动平均线
	OGLKview::Point pt_ma5 = { 0 }, pt_ma10 = { 0 }, pt_ma20 = { 0 };
	OGLKview::Point pt_ma5old = { 0 }, pt_ma10old = { 0 }, pt_ma20old = { 0 };
	//RSI
	OGLKview::Point pt_AB6 = { 0 }, pt_AB12 = { 0 }, pt_AB24 = { 0 };
	OGLKview::Point pt_AB6old = { 0 }, pt_AB12old = { 0 }, pt_AB24old = { 0 };
}
///**************END GetMarkDatatoDraw**************///


bool OGLKview::GetMarkDatatoDraw(const char* file, void* P, char* title)
{
	using namespace GMDD;
	//#if 0
	//	if (this->file != nullptr)
	//	{
	//		sWrite = this->file + std::string(_T("\\MACD.TXT"));
	//		writefile = sWrite.c_str();
	//	}
	//打开文件流
	//	Readfile.open(writefile, std::ios::out);
#ifdef _WIN32
	if (P == nullptr || (*(HWND*)P) != this->m_hDlg)
		P = &this->m_hDlg;
#endif // _WIN32
	if (file == nullptr && file == "")
		return false;
	else
	{
		if (file != "")
			this->file = const_cast<char*>(file);
		Readfile.open(this->file, std::ios::in);
	}
	if (Readfile.fail())
	{
		//只发送一遍失败消息
		if (failmsg < 1)
#ifdef _WIN32
			::PostMessage((*(HWND*)P), WM_MSG_OGL, 0, (LPARAM)this->index.AllocBuffer(_T("Reading failure!")));
#else
			cout << "Reading failure!" << endl;
#endif
		failmsg++;
		return false;
	}
	else {
		while (getline(Readfile, tmp))	// 逐行读
		{
			buff = (char*)tmp.c_str();
			token = _Strtok(buff, "/,\t", &cot);
			while (token != NULL)
			{
				markdata.push_back(token);// 获取描述
				token = _Strtok(NULL, "/,\t", &cot);
			}
			token = NULL;
			//5行数据一组，第一组7行
			if ((markdata.size() <= 7) && (markdata.size()>0))
			{
				//前2行是数据格式和说明
				if (markdata.size() < 3)
				{
					coding = true;
					buff = markdata[0];
					if (title == NULL)
					{
						title = buff;
						//memcpy();
#ifdef _WIN32
						if (failmsg <= 3)
							::PostMessage((*(HWND*)P), WM_MSG_TITL, 0, (LPARAM)this->index.AllocBuffer((CString)title));
#else
						cout << "[\033[34m" << title << "\033[0m]" << endl;
#endif // _MSC_VER
					}
					i = 0;
					token = _Strtok(buff, " ", &cot);
					while (token != NULL)	// 行优先
					{
						div_stock[i] = token;
						token = _Strtok(NULL, " ", &cot);
						i++;
					}
					SwitchViewport(0);
					sprintf(code, "%s(%s)", div_stock[1], div_stock[0]);
					DrawKtext(code, pt_code, 20, { 1,1,0 }, "Terminal", false);
					sprintf(code, ("%s(%s)<%s>"), div_stock[1], div_stock[0], div_stock[2]);
					pt_code = { -1.22f,pt_code.y };// 格式化并输出
					DrawKtext(code, pt_code, 12, { 1,1,0 }, "宋体");
					pt_code = { 0.8f,1.189f };
					*div_stock = NULL;
				}
				else { 1; }//continue;
						   // 数据的初始化就绪
				markdata.clear();// 清空数据但不释放空间以便重复利用
			}
			else if (markdata.size() > 8)
			{
				//将行情数据临时存储到结构体
				st_stock.time.tm_year = atoi(markdata[0]);
				st_stock.time.tm_mon = atoi(markdata[1]);
				st_stock.time.tm_mday = atoi(markdata[2]);
				st_stock.open = (float)atof(markdata[3]);
				st_stock.high = (float)atof(markdata[4]);
				st_stock.low = (float)atof(markdata[5]);
				st_stock.close = (float)atof(markdata[6]);
				st_stock.amount = atoi(markdata[7]);
				st_stock.price = (float)atof(markdata[8]);
				vec_market.push_back(st_stock);
				//设置初始显示图形数量
				if (line < tinkep.move + dlginfo.cycle / tinkep.ratio)
				{
					if (line > 0)//li不必分组
						if (li > 3)
						{
							li = 0;
							isnext = true;
						}
					if (line <= 3)
					{
						this->dlginfo.line = 1;
						Pter = Pt[0];
						last.stRSA._6 = last.stRSA._12 = last.stRSA._24 = atof(markdata[6]);
					}
					else
						this->dlginfo.line = line - 2;
					{//初始化
						line % 20 == 0 ? totma._20 = totma._10 = totma._5 = 0 : \
							(line % 10 == 0 ? totma._10 = totma._5 = 0 : \
							(line % 5 == 0 ? totma._5 = 0 : 1));
						idx % 24 == 0 ? frise.stRSA._24 = fdrop.stRSA._24 = 0 : \
							(idx % 12 == 0 ? frise.stRSA._12 = fdrop.stRSA._12 = 0 : \
							(idx % 6 == 0 ? frise.stRSA._6 = fdrop.stRSA._6 = 0 : 1));
						line == 24 ? pt_AB24 = Pt[li] : \
							(line == 20 ? pt_ma20old = Pt[li] : \
							(line == 12 ? pt_AB12 = Pt[li] : \
								(line == 10 ? pt_ma10old = Pt[li] : \
								(line == 6 ? pt_AB6 = Pt[li] : \
									(line == 5 ? pt_ma5old = Pt[0] : \
										Pt[i])))));
					}
					//SMA加和
					totma._5 += (atof(markdata[4]) + atof(markdata[5])) / 2;
					totma._10 += (atof(markdata[4]) + atof(markdata[5])) / 2;
					totma._20 += (atof(markdata[4]) + atof(markdata[5])) / 2;
					//比较当前组最大最小交易价格
					if (maxprice < atof(markdata[4]))
						maxprice = atof(markdata[4]);
					if (minprice > atof(markdata[5]))
						minprice = atof(markdata[5]);
					if (volume < atoi(markdata[7]))
						volume = atoi(markdata[7]);
					this->lastmarket = st_stock;
					//瞄点
					Pt[li].x =
						//22 - 2 * tinkep.ratio*(3 - line*(tinkep.ratio + 9)*0.01f + 3.f + tinkep.move*.1f);
						this->Pxtinker(tinkep);
					Pt[li].y = (float)atof(markdata[6])/**.9f*/;
#if !defined(CMfcKView)
					//					ASSERT(!_CrtCheckMemory());
#endif // !
					//计算RSA
					if (tinkep.ratio == 0)
					{
						Pt[li].x += 6.5f;
						Pt[li].y /= 2;
					}
					this->DrawPoly(Pter, Pt[li], { 0.f,1.f,0.f });
					Pter = Pt[li];
					//绘制MA线
					line < 20 ? pt_ma20old = pt_ma20 : \
						(line < 10 ? pt_ma10old = pt_ma10 : \
						(line < 5 ? pt_ma5old = pt_ma5 : pt_ma5old));
					if ((line - 1) % 20 == 0)
					{
						ma20.X = (int)totma._20;
						ma20.M = 1;
						ma20.N = 20;
						if (line - 1 == 20)
						{
							pt_ma20.y = isma._20 = totma._20 / 20;
							pt_ma20old.x = Pter.x;
							pt_ma20old.y = pt_ma20.y;
						}
						else
						{

						}
						DrawPoly(pt_ma20old, pt_ma20, { 0.9f, 0.0f, 0.9f }, 4);
						pt_ma20old = pt_ma20;
						hadraw20 = true;
					}
					else if ((line - 1) % 10 == 0)
					{
						ma10.X = (int)totma._10;
						ma10.M = 1;
						ma10.N = 10;
						pt_ma10.y = isma._10 = totma._10 / 10;
						pt_ma10old.x = Pter.x;
						pt_ma10old.y = pt_ma10.y;
						DrawPoly(pt_ma10old, pt_ma10, { 1.f, 1.0f, 0.f }, 4);
						pt_ma10old = pt_ma10;
						hadraw10 = true;
					}
					else if ((line - 1) % 5 == 0)
					{
						ma5.X = (int)totma._5;
						ma5.M = 1;
						ma5.N = 5;
						pt_ma5.y = isma._5 = totma._5 / 5;
						pt_ma5old.x = Pter.x;
						pt_ma5old.y = pt_ma5.y;
						DrawPoly(pt_ma5old, pt_ma5, { 1.f, 1.0f, 1.f }, 4);
						pt_ma5old = pt_ma5;
						hadraw5 = true;
					}
					//绘制RSA线
					pt_AB6.x = pt_AB12.x = pt_AB24.x = Pxtinker(tinkep);
					//9 - tinkep.ratio*(3 - line*(tinkep.ratio + 9)*0.01f + 3.f + tinkep.move*.1f);
					pt_AB6.y = stock.RSI(frise.stRSA._6, fdrop.stRSA._6)*8.33f;
					pt_AB12.y = stock.RSI(frise.stRSA._12, fdrop.stRSA._12)*8.33f;
					pt_AB24.y = stock.RSI(frise.stRSA._24, fdrop.stRSA._24)*8.33f;
					line < 6 ? (pt_AB6old = pt_AB6) : (line <= 12 ? (pt_AB12old = pt_AB12) : (line < 24 ? pt_AB24 = pt_AB24old : pt_AB24old));
					//RSI6粉红
					if (line % 6 == 0) {
						frise.stRSA._6 = stock.RSI(frise.stRSA._6, fdrop.stRSA._6);
						pt_AB6.y = frise.stRSA._6*8.33f;
						DrawPoly(pt_AB6old, pt_AB6, { 1.f,.75f,.8f }, 4);
						pt_AB6old = pt_AB6;
					}
					//RSI12弱红
					if (line % 12 == 0) {
						frise.stRSA._12 = stock.RSI(frise.stRSA._12, fdrop.stRSA._12);
						pt_AB12.y = frise.stRSA._12*8.33f;
						DrawPoly(pt_AB12old, pt_AB12, { .9f,.1f,.3f }, 4);
						pt_AB12old = pt_AB12;
					}
					//RSI24紫色
					if (line % 24 == 0) {
						frise.stRSA._24 = stock.RSI(frise.stRSA._24, fdrop.stRSA._24);
						pt_AB24.y = frise.stRSA._24*8.33f;
						DrawPoly(pt_AB24old, pt_AB24, { .9f,0.f,.9f }, 4);
						pt_AB24old = pt_AB24;
					}
					//SMA10:
					pt_ma5 = { -0.7f,1.183f };
					pt_ma10 = { -0.44f,1.183f };
					pt_ma20 = { -0.2f,1.183f };
					//MACD:
					pt_macd = { -1.22f,-0.44f };
					//成交量:
					pt_vol = { pt_macd.x,-0.135f };
					//DIF:
					pt_dif = { pt_macd.x + 0.32f,-0.44f };
					//DEA:
					pt_dea = { pt_dif.x + 0.2f,-0.44f };
					//RSI:
					pt_rsi = { pt_vol.x,-0.74f };
					//RSI6,12,24
					pt_rsi6 = { pt_rsi.x + 0.32f,pt_rsi.y };
					pt_rsi12 = { pt_rsi6.x + 0.32f,pt_rsi6.y };
					pt_rsi24 = { pt_rsi12.x + 0.32f,pt_rsi12.y };
					//SM5白色
					SwitchViewport(1);
					if (hadraw5) {//添加判断条件
						sprintf_s(ma, "%s%.2f", s_ma5, isma._5);
						DrawKtext(ma, pt_ma5, 15);
						hadraw5 = false;
					}
					//SM10黄色
					if (hadraw10) {
						sprintf_s(ma, "%s%.2f", s_ma10, isma._10);
						DrawKtext(ma, pt_ma10, 15, { 1.f,1.f,0.f });
						hadraw10 = false;
					}
					//SM20紫色
					if (hadraw20) {
						sprintf_s(ma, "%s%.2f", s_ma20, isma._20);
						DrawKtext(ma, pt_ma20, 15, { 1.f,0.f,1.f });
						hadraw20 = false;
					}
					//转换视口
					SwitchViewport(0);
					//成交量Vol红色
					sprintf_s(vol, "%s%d", s_vol, volume);
					DrawKtext(vol, pt_vol, 15, { 1,0,0 });
					//MACD红
					sprintf_s(macd, "%s", s_macd);
					DrawKtext(macd, pt_macd, 15, { 1,0,0 });
					//DIF深绿
					sprintf_s(dif, "%s%.2f", s_dif, fo);
					DrawKtext(macd, pt_macd, 15, { 1,0,0 });
					//DEA黄色
					sprintf_s(dea, "%s%.2f", s_dea, fo);
					DrawKtext(macd, pt_dea, 15, { 1,0,0 });
					//RSI红色
					sprintf_s(rsi, "%s%.2f", s_rsi, fo);
					DrawKtext(rsi, pt_rsi, 15, { 1,0,0 });
					memset(rsi, 0, sizeof(rsi));
					//RSI6粉红
					total.stRSA._6 = stock.RSI(frise.stRSA._6, fdrop.stRSA._6);
					sprintf_s(rsi, "RSI6:%.3f", frise.stRSA._6);
					DrawKtext(rsi, pt_rsi6, 15, { 1,0.75f,0.8f });
					memset(rsi, 0, sizeof(rsi));
					//RSI12弱红
					total.stRSA._12 = stock.RSI(frise.stRSA._12, fdrop.stRSA._12);
					sprintf_s(rsi, "RSI12:%.3f", frise.stRSA._12);
					DrawKtext(rsi, pt_rsi12, 15, { 0.9f,0.1f,0.3f });
					memset(rsi, 0, sizeof(rsi));
					//RSI12紫色
					total.stRSA._24 = stock.RSI(frise.stRSA._24, fdrop.stRSA._24);
					sprintf_s(rsi, "RSI24:%.3f", frise.stRSA._24);
					DrawKtext(rsi, pt_rsi24, 15, { 0.9f,0.f,0.9f });
					memset(rsi, 0, sizeof(rsi));
					if (line >= tinkep.move)
					{
						DrawKline(st_stock, tinkep);
					}
					//刻度视口
					//插入表格数据
					//FillChart();
					//标尺虚线
					//市场详情窗口
					if (dlginfo.drawstaff > 0)
						DrawDetail(st_stock);
					//分组计数
					idx++;
					//点计数
					li++;
				}
				markdata.clear();
			}
			else return false;
			line++;
		}
		this->dlginfo.line = line = 0;
	}
	vector<char*>().swap(markdata);
	std::vector<OGLKview::Market>().swap(vec_market);
	Readfile.close();
	return true;
}

void OGLKview::Market::show()
{
}

OGLKview::~OGLKview()
{
#ifdef __linux
	TTF_CloseFont(ttffont); //Close the font that was used
	TTF_Quit();             //Quit SDL_ttf
	SDL_Quit();             //Quit SDL
#endif
}
