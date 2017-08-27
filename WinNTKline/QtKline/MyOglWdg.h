#ifndef QMyOglWdg_H
#define QMyOglWdg_H

#include <QOglKview>
#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QEnterEvent>
#include <QDebug>
#include <Qgl>

#define GLTEST

class QMyOglWdg : public QGLWidget//, protected QOpenGLFunctions
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
