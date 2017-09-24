#include "MyOglWdg.h"

QMyOglWdg::QMyOglWdg(const char* title, bool fs)
{
	setGeometry(400, 200, 640, 480);
	fullscreen = fs;
	xPos = yPos = zPos = 0.0;
	xRate = yRate = 0.0;
	zZoom = -1.0;

	light = false;
#ifdef K_line
	setWindowTitle("K_line");
#else
	setWindowTitle(title);
#endif
	if (fullscreen)
		showFullScreen();

	initializeGL();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timerDone()));
	timer->start(100 / 24);
}

QMyOglWdg::~QMyOglWdg()
{
	if (timer->isActive())
		timer->stop();
}

void QMyOglWdg::initializeGL()
{
#ifdef  OGL_KVIEW_H_
	kv.AdjustDraw(640, 480);
#else
	loadGLTextures();
#endif
}

void QMyOglWdg::paintGL()
{
#ifdef  OGL_KVIEW_H_
	kv.InitGraph();
	kv.DrawCoord(0, 0);
	kv.GetMarkDatatoDraw("../MFCKline/data/SH600747.DAT");
	xPos += xRate;
	yPos += yRate;
	qDebug() << "(x=" << xRate << ",y=" << yRate << ",z=" << zZoom << ")";
#else

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glTranslatef(-1.0, 0.0, -8.0);
	glBegin(GL_QUADS);
	glVertex3f(0.0, 1.0, 0.0);
	glVertex3f(1.0, 1.0, 0.0);
	glVertex3f(1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glEnd();

	glTranslatef(3.0, yRate, -8.0);
	glBegin(GL_TRIANGLES);
		glColor3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 1.0, 0.0);
		glColor3f(0.0, 1.0, 0.0);
		glVertex3f(-1, -1, 0.0);
		glColor3f(0.0, 0.0, 1.0);
		glVertex3f(1.0, -1, 0.0);
	glEnd();

	if (yRate > 5.55)
		yRate = 5.55;
	if (yRate < -5.55)
		yRate = -5.55;
	qDebug() << "(x=" << xRate << ",y=" << yRate << ",z=" << zZoom << ")";
#endif
}

void QMyOglWdg::resizeGL(int width, int height)
{
	if (height == 0)
	{
		height = 1;
	}
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLdouble aspectRatio = (GLfloat)height / (GLfloat)width;
	GLdouble zNear = 0.1;
	GLdouble zFar = 100.0;

	GLdouble rFov = 45.0 * _PI_ / 180.0;
	glFrustum(-zNear * tan(rFov / 2.0) * aspectRatio,
		zNear * tan(rFov / 2.0) * aspectRatio,
		-zNear * tan(rFov / 2.0),
		zNear * tan(rFov / 2.0),
		zNear, zFar);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
}

void QMyOglWdg::keyPressEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_F2:
		fullscreen = !fullscreen;
		if (fullscreen)
		{
			showFullScreen();
		}
		else
		{
			showNormal();
			setGeometry(400, 200, 640, 480);
		}
		updateGL();
		break;
	case Qt::Key_L:
		light = !light;
		if (!light)
		{
			glDisable(GL_LIGHTING);
		}
		else
		{
			glEnable(GL_LIGHTING);
		}
		updateGL();
		break;
	case Qt::Key_Right:	//¡ú
		xRate += 0.01f;
		updateGL();
		break;
	case Qt::Key_Left:	//¡û
		xRate -= 0.01f;
		updateGL();
		break;
	case Qt::Key_Up:	//¡ü
		yRate += 0.01f;
		updateGL();
		break;
	case Qt::Key_Down:	//¡ý
		yRate -= 0.01f;
		updateGL();
		break;
	case Qt::Key_Plus:	//+
		zZoom += 0.1f;
		updateGL();
		break;	
	case Qt::Key_Minus:	//-
		zZoom -= 0.1f;
		updateGL();
		break;
	case Qt::Key_Space:
		yRate += 0.3f;
		updateGL();
		break;
	case Qt::Key_Escape:
		close();
		break;
	}
}

void QMyOglWdg::timerDone()
{
	yRate -= 0.01;
	if (yRate < 0)
		yRate = 0;
	updateGL();
	QCoreApplication::processEvents(QEventLoop::AllEvents);
}
