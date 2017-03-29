#include "MyOglWdg.h"

QMyOglWdg::QMyOglWdg(QWidget* parent, const char* name, bool fs)
	: QGLWidget(parent)
{
	fullscreen = fs;
	setGeometry(0, 0, 640, 480);
	xRot = yRot = zRot = 0.0;
	zoom = -5.0;
	xSpeed = ySpeed = 0.0;

	filter = 0;

	light = false;

	setWindowTitle("tsymiar's Tutorial");

	if (fullscreen)
		showFullScreen();
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

	kv.GetMarkDatatoDraw();
	glBindTexture(GL_TEXTURE_2D, texture[filter]);
	xRot += xSpeed;
	yRot += ySpeed;
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
	gluPerspective(45.0, (GLfloat)width / (GLfloat)height, 0.1, 100.0);
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
