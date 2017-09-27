#pragma once
#ifdef K_line
#include <QOglKview>
#endif
#if QT_VERSION >= 0x050400
#include <QtWidgets/QOpenGLWidget>
#endif
#if QT_VERSION >= 0x050600
#include <QOpenGLExtraFunctions>
#endif
#include <QCoreApplication>
#include <QtOpenGL/qgl.h>
#include <QEnterEvent>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

#ifndef _PI_
#define _PI_ 3.14159265f
#endif

class QOglMaterial : public QGLWidget
{
public:
	QOglMaterial(QWidget* parent = 0);
	virtual ~QOglMaterial();
	void loadGLTextures();
private:
	GLuint texture[3];
};
