#pragma once
#include <Qt>
#ifdef K_line
#include <QOglKview>
#endif
#if QT_VERSION >= 0x060800
#include <QOpenGLWidget>
#ifdef _WIN32
#include <QtOpenGLWidgets/qopenglwidget.h>
#endif
#elif QT_VERSION >= 0x050400
#ifdef _WIN32
#pragma execution_character_set("utf-8")
#endif
#include <QtWidgets/QOpenGLWidget>
#else
#error Only support QT version 5.4+, while current is under v5.4.
#endif
//#include <QOpenGLFunctions_4_3_Compatibility>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QMainWindow>
#include <QEnterEvent>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include "OglImgShow.h"
#include "SDL2tex.h"

#ifndef _PI_
#define _PI_ 3.141592653589793f
#endif
#define EPSILON 0.000001

//#define _GLVBO_

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
#ifdef _GLVBO_
    void initVbo();
    inline QMatrix4x4 getProject() const { return m_projection; }
private:
    /* [1] 定义着色器和片段着色器，否则做不了任何渲染 */
    /* 定义一个着色器[顶点着色器、片段着色器]编译对象 */
    QOpenGLShaderProgram* program;
    ///< 视图矩阵、投影矩阵、MVP矩阵
    ///< 分三个矩阵，分别是模型矩阵、视图矩阵、透视矩阵:
    ///  1. 单独控制模型灯光跟随，shader要传入除了mvp矩阵外的模型矩阵*视图矩阵
    QMatrix4x4 m_projection;
    ///< 可以根据programid，利用glGetUniformLocation等方法获取shader属性
    GLuint programid;

    ///< 矩阵、顶点、颜色在着色器里的位置
    GLuint matrixLocation, vertexLocation, clorLocation;

    ///< 顶点、索引、颜色 -> buffer的标识
    GLuint verVbo, v_indexVbo, colorVbo;
#else
private:
#ifdef OGL_KVIEW_H_
    OGLKview kv;
#endif
#endif
    GLfloat xVal, yVal, zZoom, tHigh;
    int mX, mY;
    OglImgShow mPng;
    void coord();
    QString text;
    void textOut(int left = 10, int upon = 40, QColor color = Qt::yellow, float th = 1, QString family = NULL);
    bool bingo = false;
    bool m_showSdl = false;
protected:
    inline void setXval(GLfloat x) { xVal = x; }
    inline void setYval(GLfloat y) { yVal = y; }
    inline void setZoom(GLfloat z) { zZoom = z; }
    inline void setHigh(GLfloat h) { tHigh = h; }
    inline void setXloc(int x) { mX = x; }
    inline void setYloc(int y) { mY = y; }
    inline GLfloat getHeight() { return yVal + 0.5f; }
    inline void setText(QString text) { this->text = text; }
    inline void setBingo(bool bingo = true) { this->bingo = bingo; }
public:
    inline void setSdlWin(bool show = true) { m_showSdl = show; }
};
