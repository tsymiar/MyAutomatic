#ifndef QMyOglWdg_H
#define QMyOglWdg_H

#include <QOglKview>
#include <QtWidgets/QOpenGLWidget>
#include <QEnterEvent>
#include <Qgl>

#define GLTEST

class QMyOglWdg : public QGLWidget
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
	OGLKview kv;
};

#endif // QMyOglWdg_H
