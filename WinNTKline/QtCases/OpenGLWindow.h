#ifndef OpenGLWindow_H
#define OpenGLWindow_H

#include "OglMaterial.h"

class OpenGLWindow : public QOglMaterial //, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	OpenGLWindow(const char* title = "tsymiar's Tutorial", bool fs = false);
	~OpenGLWindow();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void keyPressEvent(QKeyEvent *e);
private:
	bool fullscreen;
	bool light;
	GLfloat xPos, yPos, zPos;
	GLfloat xRate, yRate;
	GLfloat zZoom;
	QTimer *timer = NULL;
#ifdef OGL_KVIEW_H_
	OGLKview kv;
#endif
private slots:
	void timerDone();
};

#endif // OpenGLWindow_H
