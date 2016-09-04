#ifndef QMyOglWdg_H
#define QMyOglWdg_H

#include <QtWidgets/QOpenGLWidget>

class QMyOglWdg : public QOpenGLWidget
{
	Q_OBJECT

public:
	QMyOglWdg(QWidget *parent = 0);
	~QMyOglWdg();

private:

};

#endif // QMyOglWdg_H
