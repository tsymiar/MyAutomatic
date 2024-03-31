#ifndef OpenGLWindow_H
#define OpenGLWindow_H

// #include "ui_openglwindow.h"
#include "OglMaterial.h"

class OpenGLWindow : public QOglMaterial
{
    Q_OBJECT

public:
    OpenGLWindow(const char* title = "tsymiar's Tutorial", bool fs = false);
    ~OpenGLWindow();
protected:
    void keyPressEvent(QKeyEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
private:
    bool fullscreen;
    bool light;
    GLfloat x, y, z, h;
    QTimer *timer = NULL;
private slots:
    void timeTickDone();
};

#endif // OpenGLWindow_H
