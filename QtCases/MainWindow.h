#ifndef MainWindow_H
#define MainWindow_H

// #include "ui_mainwindow.h"
#include "OglMaterial.h"

class MainWindow : public QOglMaterial {
#if (! defined _WIN32)
    Q_OBJECT
#endif
public:
    MainWindow(const char* title = "tsymiar's Tutorial", bool fs = false);
    ~MainWindow();
protected:
    void keyPressEvent(QKeyEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent *e) override;
private:
    bool light;
    bool fllscrn;
    GLfloat x, y, z, h;
    QTimer* tick = NULL;
private slots:
    void timeTickDone();
};

#endif // MainWindow_H
