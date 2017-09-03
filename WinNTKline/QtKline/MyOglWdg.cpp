#include "MyOglWdg.h"

QMyOglWdg::QMyOglWdg(QWidget* parent, const char* name, bool fs)
	: QGLWidget(parent)
{
	setGeometry(400, 200, 640, 480);
	fullscreen = fs;
	xRot = yRot = zRot = 0.0;
	xSpeed = ySpeed = 0.0;
	zoom = -5.0;

	filter = 0;
	light = false;

	setWindowTitle("tsymiar's Tutorial");

	if (fullscreen)
		showFullScreen();

	initializeGL();
}

QMyOglWdg::~QMyOglWdg()
{

}

void QMyOglWdg::initializeGL()
{
	kv.InitGraph();
}

void QMyOglWdg::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
#if !defined(GLTEST)
	kv.DrawCoord(xRot, yRot);
	kv.GetMarkDatatoDraw();
	//glBindTexture(GL_TEXTURE_2D, texture[filter]);
#else
	xRot += xSpeed;
	yRot += ySpeed;
	qDebug() << "(x=" << xRot << ",y=" << yRot << ",z=" << zoom << ")";

	glTranslatef(-1, 0.0, -8.0);
	glBegin(GL_QUADS);
	glVertex3f(-1.0, 1.0, 0.0);
	glVertex3f(1.0, 1.0, 0.0);
	glVertex3f(1.0, -1.0, 0.0);
	glVertex3f(-1.0, -1.0, 0.0);
	glEnd();

	glTranslatef(3.0, 0.0, 0.0);
	glBegin(GL_TRIANGLES);
	
		glColor3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 1.0, 0.0);
		glColor3f(0.0, 1.0, 0.0);
		glVertex3f(-1, -1, 0.0);
		glColor3f(0.0, 0.0, 1.0);
		glVertex3f(1.0, -1, 0.0);
	glEnd();
#endif
}

void QMyOglWdg::resizeGL(int width, int height)
{
	if (height == 0)
	{
		height = 1;
	}
	glViewport(0, 0, (GLint)width, (GLint)height);
	//gluPerspective(45.0, (GLfloat)width / (GLfloat)height, 0.1, 100.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLdouble aspectRatio = (GLfloat)width / (GLfloat)height;
	GLdouble zNear = 0.1;
	GLdouble zFar = 100.0;

	GLdouble rFov = 45.0 * 3.14159265 / 180.0;
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
			setGeometry(0, 0, 640, 480);
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
	case Qt::Key_F:
		filter += 1;;
		if (filter > 2)
		{
			filter = 0;
		}
		updateGL();
		break;
	case Qt::Key_Period:
		zoom -= 0.2;
		updateGL();
		break;
	case Qt::Key_PageUp:
		zoom += 0.2;
		updateGL();
		break;
	case Qt::Key_Up:
		xSpeed -= 0.01;
		updateGL();
		break;
	case Qt::Key_Down:
		xSpeed += 0.01;
		updateGL();
		break;
	case Qt::Key_Right:
		ySpeed += 0.01;
		updateGL();
		break;
	case Qt::Key_Left:
		ySpeed -= 0.01;
		updateGL();
		break;
	case Qt::Key_Escape:
		close();
	}
}

void QMyOglWdg::loadGLTextures()
{
	QImage tex, buf;
	if (!buf.load("./data/test.bmp"))
	{
		qWarning("Could not read image file, using single-color instead.");
		QImage dummy(128, 128, QImage::Format_RGB32);
		dummy.fill(QColor("green").rgb());
		buf = dummy;
	}
	tex = QGLWidget::convertToGLFormat(buf);

	glGenTextures(3, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, tex.width(), tex.height(), 0,
		GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, tex.width(), tex.height(), 0,
		GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
}
