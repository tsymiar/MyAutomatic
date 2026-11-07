#pragma once
#include <QtGlobal>
#ifdef K_line
#include <QOglKview>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLWidget>
#elif QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
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
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
#ifdef _GLVBO_
    void initVbo();
    inline QMatrix4x4 getProject() const { return m_projection; }
private:
    QOpenGLShaderProgram* program;
    // 透视投影矩阵
    QMatrix4x4 m_projection;
    GLuint programid;
    GLuint matrixLocation, vertexLocation, clorLocation;
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
    float bgScrollU = 0.0f;      // 背景水平滚动偏移 (0.0 ~ 1.0)
    static constexpr float BG_MAX_SCROLL = 0.44f; // 最大滚动范围，可调
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
