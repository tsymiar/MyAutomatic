#pragma once
#include <Qt>
#ifdef K_line
#include <QOglKview>
#endif
#if QT_VERSION >= 0x050400
#include <QtWidgets/QOpenGLWidget>
#else
#error Only support QT version 5.4+, while current is under v5.4.
#endif
//#include <QOpenGLFunctions_4_3_Compatibility>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QEnterEvent>
#include <QEventLoop>
#include <QTimer>
#include <gl/GLU.h> 
#include <gl/GL.h> 
#include <QDebug>

#ifndef _PI_
#define _PI_ 3.14159265f
#endif

//#define _VBO_

class QOglMaterial : public QOpenGLWidget, protected QOpenGLFunctions // QOpenGLFunctions_4_3_Compatibility
{
public:
	QOglMaterial(QWidget* parent = 0);
	virtual ~QOglMaterial();
protected:
	// virtual functions
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void loadGLTextures();
#ifdef _VBO_
	void initVbo();
	inline QMatrix4x4 getProject() const { return m_projection; }
private:
	/* [1] 定义着色器和片段着色器，不然做不了任何渲染 */
	/*   定义一个着色器[顶点着色器、片段着色器]编译对象 */
	QOpenGLShaderProgram * program;
	///< 视图矩阵、投影矩阵、MVP矩阵
	///< 分三个矩阵，分别是模型矩阵、视图矩阵、透视矩阵:
	///  1. 单独控制模型灯光跟随，shader要传入除了mvp矩阵外的模型矩阵*视图矩阵
	QMatrix4x4 m_projection;
	///< 可以根据此id，利用glGetUniformLocation等方法获取shader里的属性
	GLuint programid;

	///< 矩阵、顶点、颜色在着色器里的位置
	GLuint matrixLocation, vertexLocation, clorLocation;

	///< 顶点、索引、颜色 -> buffer的标识
	GLuint verVbo, v_indexVbo, colorVbo;
#else
#ifdef OGL_KVIEW_H_
	private：
		OGLKview kv;
#else
	inline void setX(GLfloat x) { xRate = x; }
	inline void setY(GLfloat y) { yRate = y; }
	inline void setZ(GLfloat z) { zZoom = z; }
	inline void setS(GLfloat s) { sHigh = s; }
private:
	GLuint texture[3];
	GLfloat xRate, yRate, zZoom, sHigh;
#endif
#endif
};
