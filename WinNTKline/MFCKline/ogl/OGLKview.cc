#include "OGLKview.h"

OGLKview::OGLKview()
{
	coding = false;
	radius = 1.0f;
	unfurl = false;
}

char m_time[32] = {NULL};
char l_time[32] = {NULL};
OGLKview::OglAttr attr = {0};
OGLKview::Item m_ITEM = { 0,0,0,NULL };
OGLKview::ViewSize viewsize = { 0,0,0,0 };
OGLKview::Charmarket markchar = { "---","---","---" };

#ifdef __linux//||_UNIX
int main(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	glutInitWindowSize(1280, 750);
	glutInitWindowPosition(640, 375);

	glutCreateWindow("OGLKline");

	glutReshapeFunc(AdjustDraw);
	glutDisplayFunc(InitFunc);

	glutMainLoop();
	return(0);
}
#else
#ifdef _MSC_VER
bool OGLKview::SetWindowPixelFormat(HDC m_hDC, HWND m_hWnd, int pixelformat)
{
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),      // pfd的大小
		1,                                  // 版本号
		PFD_DRAW_TO_WINDOW |                // 支持窗口
		PFD_SUPPORT_OPENGL |                // 支持OpenGL
		PFD_DOUBLEBUFFER,                   // 双缓存
		PFD_TYPE_RGBA,                      // RGBA类型
		24,									// 24位颜色深度
		0,0,0,0,0,0,                        // 忽略颜色位
		0,                                  // 无alpha缓存
		0,                                  // 忽略转换位
		0,                                  // 无累计缓存
		0,0,0,0,                            // 忽略累计位
		32,									// 32位深度缓存
		0,                                  // 无模版缓存
		0,                                  // 无辅助缓存
		PFD_MAIN_PLANE,                     // 主层（main layer）
		0,                                  // 保留（reserved）
		0,0,0,
	};
	if (!(pixelformat = ChoosePixelFormat(m_hDC, &pfd)))
	{
		::PostMessage(m_hWnd,WM_MSG_OGL,0,(LPARAM)index.AllocBuffer(_T("Choose Pixel Format failed !")));
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
	GetConsoleWindow = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, _T("GetConsoleWindow"));
	return true;
}
//unsigned _stdcall iItem(void* p)
//{
//	OGLKview* kv = (OGLKview*)p;
//	TH *th = (TH*)p;
//}
float Xdeg, Ydeg;
bool multi0 = false;
OGLKview::Point mpt,arrow;
OGLKview::Market fixeddata;
float proportion, delta = 1;
float shadowup, shadowdown;
float candlemiddle, candleright, candleleft;
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
	sprintf_s(l_time, "%d-%d-%d", markdata.time.tm_yday, markdata.time.tm_mon, markdata.time.tm_mday);
	tinkep.datacol = dlginfo.line;// -3;
	hollow = !dlginfo.bkg;
	mpt.x = (float)dlginfo.mouX;
	mpt.y = (float)dlginfo.mouY;
	candlemiddle = Xdeg = pt.x = Okv->Pxtinker(co);
	fillitem.closing	= markdata.close;
	fillitem.curprice	= markdata.price;
	fillitem.gross		= markdata.amount;
	fillitem.toopen		= markdata.open;
	fillitem.upadp		= markdata.close - markdata.open;
	fillitem.closed		= lastmarket.close;
	markdata.open != 0 ? \
		(fillitem.uprange = fillitem.upadp / markdata.open) : 0;
	sprintf(markchar.close,	"%.2f", markdata.close);
	sprintf(markchar.price,	"%.2f", markdata.price);
	sprintf(markchar.amount,"%.2f", markdata.amount);
	sprintf(markchar.open,	"%.2f", markdata.open);
	sprintf(markchar.high,	"%.2f", markdata.high);
	sprintf(markchar.low,	"%.2f", markdata.low);
	if (fillitem.higest < markdata.high)
		fillitem.higest = markdata.high;
	if (fillitem.lowest < markdata.low)
		fillitem.lowest = markdata.low;
	//
	if (tinkep.ratio < 1)
		multi0 = true;
	else
		multi0 = false;
	fixeddata.open	= Pytinker(markdata.open,	tinkep);
	fixeddata.close = Pytinker(markdata.close,	tinkep);
	fixeddata.high	= Pytinker(markdata.high,	tinkep);
	fixeddata.low	= Pytinker(markdata.low,	tinkep);
	if (fillitem.upadp > 0)
	{
		limitup = true;
		glColor3f(1, 0, 0);
		shadowup	= fixeddata.close;
		shadowdown	= fixeddata.open;
	}
	else if (fillitem.upadp < 0)
	{
		limitup = false;
		if (dlginfo.bkg)
			glColor3f(0, 1, 1);
		else
			glColor3f(0.1f, 0.7f, 0.3f);
		shadowup	= fixeddata.open;
		shadowdown	= fixeddata.close;
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
	if (multi0)
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
				assert(_CrtCheckMemory());
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
	if ((xytinker(mpt).x >= candleleft) && (xytinker(mpt).x <= candleright))
	{
		glColor3f(0.902f, 0.902f, 0.980f);
		glPointSize(3);
		glDisable(GL_DEPTH_TEST);
		glBegin(GL_POINTS);
		glVertex2f(candlemiddle, Pytinker(markdata.close, tinkep));
		glEnd();
		glEnable(GL_DEPTH_TEST);
		memcpy(markchar.time, m_time, strlen(m_time));
		markchar.time[strlen(m_time)] = '\0';
		sprintf(markchar.time,	"%d.%d.%d", markdata.time.tm_year, markdata.time.tm_mon, markdata.time.tm_mday);
		sprintf(markchar.amount,"%d",		markdata.amount);
		sprintf(markchar.price,	"%.0f",		markdata.price);
		sprintf(markchar.close, "%.2f",		markdata.close);
		sprintf(markchar.high,	"%.2f",		markdata.high);
		sprintf(markchar.low,	"%.2f",		markdata.low);
		sprintf(markchar.open,	"%.2f",		markdata.open);
		sprintf(markchar.ydeg,	"y=%.3f",	Ydeg);
	}
	if ((xytinker(mpt).x >= fixeddata.low) && (xytinker(mpt).x <= fixeddata.high))
	{
		sprintf(markchar.hintx, "%.2f", markdata.close);
		sprintf(markchar.y, "K(y)=%.2f", xytinker(mpt));
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
			glVertex2f(candleleft,	fixeddata.low);
			glTexCoord2f(1.f, 1.f);
			glVertex2f(candleleft,	float(fixeddata.high + fixpixely));
			glTexCoord2f(0.f, 1.f);
			glVertex2f(candleright, float(fixeddata.high + fixpixely));
		}
		glEnd();
	}
	//成交量Vol
	SwitchViewport(2);
	float diff = fillitem.closing - fillitem.closed;
	fillitem.closing > fillitem.closed ? glColor3f(1, 0, 0) : \
		glColor3f(0, 1, 1);
	float voly = (markdata.amount*pow(10.0f, -8) - 0.7f)*Zoom.z_vol;
	if (multi0)
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
			glVertex2f(candleleft,	-10);
			glVertex2f(candleleft,	voly);
			glVertex2f(candleleft,	voly);
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
#endif
#endif

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
#if !defined(GLTEST)
	chart_frame();
	diag_staff(dlginfo.mouX, dlginfo.mouY);
#else
	buset.m_boostest();
#endif
#endif // DEBUG
#ifdef _CONSOLE||_WINDOWS
	SetConsoleCtrlHandler(dos.ConsoleHandler, TRUE);
#endif
	//SwapBuffers(m_hDC);
}

void OGLKview::draw_string(const char* str)
{
	int len = 0, i;
	wchar_t* wstring;
	HDC m_hDC = wglGetCurrentDC();
	GLuint list = glGenLists(1);
	for (i = 0; str[i] != '\0'; ++i)
	{
		if (IsDBCSLeadByte(str[i]))
			++i;
		++len;
	}
	if((wstring = (wchar_t*)malloc((len + 1)*sizeof(wchar_t)))==NULL)
		return;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wstring, len);
	wstring[len] = _T('\0');
	if ((wstring[0] > 0x7f)||(47 < wstring[0] && wstring[0] <= 127))
	//if (wstring[0] > 127)
		i = 0;
	else
		i = -1;//ASCII
	for (; i < len; ++i)
	{
		wglUseFontBitmapsW(m_hDC, wstring[i], 1, list);
		glCallList(list);
	}
	free(wstring);
	glDeleteLists(list, 1);
}

int OGLKview::diag_staff(int x,int y)
{
	OGLKview::Point pt = \
	{ pt.x = (float)x,pt.y = (float)y };
	glColor3f(1, 1, 1);
	glViewport(0, viewsize.pty, viewsize.ptw, viewsize.pth);
	glBegin(GL_LINES);
	{
		glVertex2f(-10.0f, this->xytinker(pt).y);
		glVertex2f(10.0f, this->xytinker(pt).y);
	}
	glEnd();
	glViewport(0, 0, viewsize.ptw, int(viewsize.pth*1.966f));
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
	m_ITEM.pc_ = market.price;
	glViewport(0, 100, attr.wide, attr.tall - 100);
	pt.x = 0.73f;
	pt.y = 0.6f - 0.08f;
	//if (pt.y <= -0.9f)pt.y = 0.6f;
	m_ITEM.mode = 1;
	sprintf_s(listime, "%s", m_time);
	view.DrawKtext(listime, pt,13);
	pt.x = 0.9f;
	sprintf_s(info, "%.2f", m_ITEM.pc_);
	view.DrawKtext(info, pt,13);
	pt.x = 1.13f;
	_itoa_s(m_ITEM.mode, info, 10);
	view.DrawKtext(info, pt, 13);
	pt.x = 1.17f;
	if (tmp)
	{
		m_ITEM.bs = "B";
		glColor3f(1, 0, 0);
	}
	else 
	{
		m_ITEM.bs = "S";
		glColor3f(0, 1, 0);
	}
	view.DrawKtext(m_ITEM.bs, pt, 13);
	return tmp;
}

void OGLKview::AdjustDraw(GLsizei W, GLsizei H, bool b)
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
	SetBkg(b);
}

int OGLKview::DrawCoord(int mX, int mY)
{
	int	msgid[4];
	char coor[16];
	char brand[64];
	char cpuattr[64];
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
	cpu_getbrand(brand);
	__cpuidex(msgid, 0, 0);
	sprintf(cpuattr, "CPU0:%d-%s", msgid[1], brand);
	pt = { -0.9f,1.19f };
	DrawKtext(cpuattr, pt, 14, { 1,1,0 });
	return m7;
}

int OGLKview::DrawArrow(OGLKview::Point begin)
{
	enum VAO_IDs { Triangles, NumVAOs };
	enum Buffer_IDs { ArrayBuffer, NumBuffers };
	enum Attrib_IDs { vPosition = 0 };
	GLuint  VAOs[NumVAOs];
	GLuint  Buffers[NumBuffers];
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

	return NumVertices;
}

int OGLKview::DrawDetail(OGLKview::Market market)
{
	float y = 0;  
	char hintx[16];
	float min = 0, step = 1, fold = 1;
	OGLKview::Point poi, iop;
	poi.x = 1.234567f;
	poi.y = 1.17f;
	iop.x = (float)dlginfo.mouX;
	iop.y = (float)dlginfo.mouY;
	min += (417 - dlginfo.mouY)*(float)fixpixely*step / fold*3.9f;
	sprintf(hintx, "%.2f", min);
	glViewport(0, pory, widt, heig);
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

		glVertex3f(-1.3f, 0.333f, 0);
		glVertex3f(-1.03f, 1.2417f, 0);

		glVertex3f(-1.03f, 1.2417f, 0);
		glVertex3f(-1.03f, 0.333f, 0);
	glEnd();

	DrawKtext(l_time, poi, 17, { 0.6f, 0.7f, 0.8f, 0.9f }, "微软雅黑");
	poi.y = 1.09f;
	DrawKtext("开盘", poi, 13, { 1,1,1 }, "宋体");
	poi.y = 0.9f;
	DrawKtext("最高", poi, 13, { 1,1,1 }, "宋体");
	poi.y = 0.7f;
	DrawKtext("最低", poi, 13, { 1,1,1 }, "宋体");
	poi.y = 0.5f;
	DrawKtext("收盘", poi, 13, { 1,1,1 }, "宋体");
	//方框
	//if((fixxy(iop).x>marketraw.left)&&(fixxy(iop).x>marketraw.right))
	//{
	poi.y = 1.0f;
	Color4f f4;
	if (limitup)
		f4 = { 1,0,0 };
	else
		f4 = { 0,1,0 };
	DrawKtext(markchar.open, poi, 13, f4, "微软雅黑");
	poi.y = 0.8f;
	DrawKtext(markchar.high, poi, 13, f4, "微软雅黑");
	poi.y = 0.6f;
	DrawKtext(markchar.low, poi, 13, f4, "微软雅黑");
	poi.y = 0.4f;
	DrawKtext(markchar.close, poi, 13, f4, "微软雅黑");
	//}
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
	if(b)
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
	case 0:
		glViewport(0, 0, attr.wide*adjust.ptw, adjust.pth*attr.tall);
		break;
	case 1:
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
	Indexes idx;
	for (int i = 0; i <= 3; i++) 
		A[i] = xytinker(A[i]);
	glColor3f(1, 1, 1);
	Point Pold = { A[0].x,A[0].y };
	for (double t = 0.f; t <= 1.f; t += .1f)
	{
		Point P = idx.CubicBézier(A, t);
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
		toe.y = i+0.07f;
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
	sprintf(listitem, "%s\t%f\t%d", m_time, fillitem.closed, 1);
	OGLKview::DrawKtext(listitem, itempt, 15);
	item0++;
}

void OGLKview::DrawKtext(char text[], Point & coor, int size, OGLKview::Color4f color, char font[], bool dim)
{
	int fw;
	if (dim)
		fw = FW_MEDIUM;
	else
		fw = FW_SEMIBOLD;
	HFONT mhfont = CreateFont(size, 0, 0, 0, fw, 0, 0, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, font);
	HFONT hOldFont = (HFONT)SelectObject(wglGetCurrentDC(), mhfont);
	DeleteObject(hOldFont);
	if (color.A == 0)
		color.A = 1;
	glColor4f(color.R, color.G, color.B, color.A);
	glRasterPos2f(coor.x, coor.y);
	draw_string(text);
}

void OGLKview::Market::show()
{
}

OGLKview::~OGLKview()
{
	
}