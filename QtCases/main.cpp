#include <QtPlugin>
#include <QtWidgets/qmessagebox.h>
#include "OpenGLWindow.h"

// Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#ifdef _WIN32
extern "C" __declspec(dllexport)
#endif
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OpenGLWindow w("flappy TRIANGLE");
    a.setActiveWindow(&w);
    w.show();
    return a.exec();
}
