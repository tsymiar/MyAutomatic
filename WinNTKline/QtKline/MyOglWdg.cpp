#include "MyOglWdg.h"

QMyOglWdg::QMyOglWdg(QWidget* parent, const char* name, bool fs)
	: QGLWidget(parent)
{
	setGeometry(400, 200, 640, 480);
	fullscreen = fs;
	xRot = yRot = zRot = 0.0;
	xSpeed = ySpeed = 0.0;
	zoom = -1.0;

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
#ifdef  OGL_KVIEW_H_
	kv.AdjustDraw(640, 480);
#else
    glShadeModel(GL_SMOOTH);  
    glClearColor( 0.0, 0.0, 0.0, 0.0 );  
    glClearDepth( 1.0 );  
    glEnable(GL_DEPTH_TEST);  
    glDepthFunc(GL_LEQUAL);  
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  
#endif
}

void QMyOglWdg::paintGL()
{
#ifdef  OGL_KVIEW_H_
	kv.InitGraph();
	kv.DrawCoord(0, 0);
	kv.GetMarkDatatoDraw("../MFCKline/data/SH600747.DAT");
	//glBindTexture(GL_TEXTURE_2D, texture[filter]);
	xRot += xSpeed;
	yRot += ySpeed;
	qDebug() << "(x=" << xSpeed << ",y=" << ySpeed << ",z=" << zoom << ")";
#else
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

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
	qDebug() << "(x=" << xSpeed << ",y=" << ySpeed << ",z=" << zoom << ")";
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
	case Qt::Key_Up:	//¡ü
		xSpeed += 0.01f;
		updateGL();
		break;
	case Qt::Key_Down:	//¡ý
		xSpeed -= 0.01f;
		updateGL();
		break;
	case Qt::Key_Right:	//¡ú
		ySpeed += 0.01f;
		updateGL();
		break;
	case Qt::Key_Left:	//¡û
		ySpeed -= 0.01f;
		updateGL();
		break;
	case Qt::Key_Period:	//.
		zoom += 0.1f;
		updateGL();
		break;	
	case Qt::Key_Comma:	//,
		zoom -= 0.1f;
		updateGL();
		break;
	case Qt::Key_Escape:
		close();
		break;
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
