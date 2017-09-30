#ifndef OpenGLWindow_H
#define OpenGLWindow_H

#include "OglMaterial.h"

class OpenGLWindow : public QOglMaterial
{
	Q_OBJECT

public:
	OpenGLWindow(const char* title = "tsymiar's Tutorial", bool fs = false);
	~OpenGLWindow();
protected:
	void keyPressEvent(QKeyEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
private:
	bool fullscreen;
	bool light;
	GLfloat xPos, yPos, zPos, sPos;
	QTimer *timer = NULL;
	private slots:
	void timerDone();
};

#endif // OpenGLWindow_H
