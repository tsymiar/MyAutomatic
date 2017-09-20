#ifndef QMyOglWdg_H
#define QMyOglWdg_H

#include "Material.h"

class QMyOglWdg : public QMaterial //, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	QMyOglWdg(const char* title = "tsymiar's Tutorial", bool fs = false);
	~QMyOglWdg();

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
private slots:
	void timerDone();
#ifdef OGL_KVIEW_H_
	OGLKview kv;
#endif
};

#endif // QMyOglWdg_H
