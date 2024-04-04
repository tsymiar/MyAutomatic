#ifndef MainWindow_H
#define MainWindow_H

// #include "ui_mainwindow.h"
#include "OglMaterial.h"

class MainWindow : public QOglMaterial {
    Q_OBJECT

public:
    MainWindow(const char* title = "tsymiar's Tutorial", bool fs = false);
    ~MainWindow();
protected:
    void keyPressEvent(QKeyEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
private:
    bool fullscrn;
    bool light;
    GLfloat x, y, z, h;
    QTimer* tick = NULL;
private slots:
    void timeTickDone();
};

#endif // MainWindow_H
