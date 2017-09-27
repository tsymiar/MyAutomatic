#include "OpenGLWindow.h"

OpenGLWindow::OpenGLWindow(const char* title, bool fs)
{
	setGeometry(400, 200, 640, 480);
	fullscreen = fs;
	xPos = yPos = zPos = sPos = 0.0;

	light = false;
#ifdef K_line
	setWindowTitle("K_line");
#else
	setWindowTitle(title);
#endif
	if (fullscreen)
		showFullScreen();
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timerDone()));
	timer->start(100 / 24);
}

OpenGLWindow::~OpenGLWindow()
{
	if (timer->isActive())
		timer->stop();
}

void OpenGLWindow::keyPressEvent(QKeyEvent * e)
{
	qDebug() << "+++ keyPressEvent" << "(" << QString("%1").arg(e->key()) << ") +++";
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
		break;
	case Qt::Key_Right:	//→
		xPos += 0.01f;
		setX(xPos);
		break;
	case Qt::Key_Left:	//←
		xPos -= 0.01f;
		setX(xPos);
		break;
	case Qt::Key_Up:	//↑
		yPos += 0.01f;
		setY(yPos);
		break;
	case Qt::Key_Down:	//↓
		yPos -= 0.01f;
		setY(yPos);
		break;
	case Qt::Key_Plus:	//+
		zPos += 0.1f;
		setZ(zPos);
		break;
	case Qt::Key_Minus:	//-
		zPos -= 0.1f;
		setZ(zPos);
		break;
	case Qt::Key_Space:
		sPos += 0.3f;
		setS(sPos);
		break;
#ifdef _VBO_
	case Qt::Key_A:
		getProject().rotate(1, 0, 1, 0);
		break;
	case Qt::Key_D:
		getProject().rotate(-1, 0, 1, 0);
		break;
	case Qt::Key_W:
		getProject().rotate(1, 1, 0, 0);
		break;
	case Qt::Key_S:
		getProject().rotate(-1, 1, 0, 0);
		break;
#endif
	case Qt::Key_Escape:
		close();
		break;
	}
	update();
}

void OpenGLWindow::timerDone()
{
	sPos -= 0.01f;
	if (sPos < -1)
		sPos = -1;
	if (sPos > 5.55)
		sPos = 5.55;
	setS(sPos);
	update();
	QCoreApplication::processEvents(QEventLoop::AllEvents);
}
