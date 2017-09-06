#ifndef QMyOglWdg_H
#define QMyOglWdg_H

#if QT_VERSION >= 0x050400
#include <QtWidgets/QOpenGLWidget>
#endif
#if QT_VERSION >= 0x050600
#include <QOpenGLExtraFunctions>
#endif
#include <QtOpenGL/qgl.h>
#include <QOglKview>
#include <QEnterEvent>
#include <QDebug>

#define GLTEST

class QMyOglWdg : public QGLWidget //, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	QMyOglWdg(QWidget* parent = 0, const char* name = 0, bool fs = false);
	~QMyOglWdg();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void keyPressEvent(QKeyEvent *e);
	void loadGLTextures();
private:
	bool fullscreen;
	bool light;
	GLfloat xRot, yRot, zRot;
	GLfloat zoom;
	GLfloat xSpeed, ySpeed;
	GLuint texture[3];
	GLuint filter;
#ifdef OGL_KVIEW_H_
	OGLKview kv;
#endif
};

#endif // QMyOglWdg_H
