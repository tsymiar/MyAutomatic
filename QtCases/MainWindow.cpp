#include "MainWindow.h"

MainWindow::MainWindow(const char* title, bool fs)
{
    setGeometry(400, 200, 287, 512);
    fullscrn = fs;
    x = y = z = h = 0.0;

    light = false;
#ifdef K_line
    setWindowTitle("K_line");
#else
    setWindowTitle(title);
#endif
    setWindowIcon(QIcon("qrc:/WinNTKline/image/qtlogo.ico"));
    if (fullscrn)
        showFullScreen();
    tick = new QTimer(this);
    connect(tick, SIGNAL(timeout()), this, SLOT(timeTickDone()));
    tick->start(100 / 24);
    setMouseTracking(true);
}

MainWindow::~MainWindow()
{
    if (tick->isActive()) {
        tick->stop();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    qDebug() << "+++ key(" << e->key() << ")" <<
        QString("x=%1 y=%2 z=%3")
        .arg(x)
        .arg(y)
        .arg(z)
        << "+++";
    switch (e->key()) {
    case Qt::Key_F2:
        fullscrn = !fullscrn;
        if (fullscrn) {
            showFullScreen();
        } else {
            showNormal();
            setGeometry(400, 200, 640, 480);
        }
        break;
    case Qt::Key_L:
        light = !light;
        if (!light) {
            glDisable(GL_LIGHTING);
        } else {
            glEnable(GL_LIGHTING);
        }
        break;
#ifdef _GLVBO_
    case Qt::Key_A:
        getProject().rotate(1, 0, 1, 0);
        break;
    case Qt::Key_D:
        getProject().rotate(-1, 0, 1, 0);
        break;
    case Qt::Key_W:
        getProject().rotate(1, 1, 0, 0);
        break;
    case Qt::Key_S:
        getProject().rotate(-1, 1, 0, 0);
        break;
#else
    case Qt::Key_Right:   //→
        x += 0.01f;
        setXval(x);
        break;
    case Qt::Key_Left:    //←
        x -= 0.01f;
        setXval(x);
        break;
    case Qt::Key_Up:      //↑
        y += 0.01f;
        setYval(y);
        break;
    case Qt::Key_Down:    //↓
        y -= 0.01f;
        setYval(y);
        break;
    case Qt::Key_Plus:    //+
        z += 0.1f;
        setZoom(z);
        break;
    case Qt::Key_Minus:   //-
        z -= 0.1f;
        setZoom(z);
        break;
    case Qt::Key_Space:
        if (fabs(h - getH()) < 0.01)
            setBingo(false);
        h += 0.3f;
        setHigh(h);
        break;
#endif
    case Qt::Key_Escape:
        close();
        break;
    }
    update();
}

void MainWindow::mouseMoveEvent(QMouseEvent* e)
{
    if (e->MouseButtonDblClick == e->type()) {
        qDebug() << "DblClick!";
    }
    setXloc(e->x());
    setYloc(e->y());
}

void MainWindow::timeTickDone()
{
#if (!defined _GLVBO_)
    h -= 0.01f;
    if (h < -1)
        h = -1;
    if (h > 5.55)
        h = 5.55;
    setHigh(h);
    if (fabs(h - getH()) < 0.01) {
        setText("bingo!");
        setBingo();
    }
    update();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
#endif
}
