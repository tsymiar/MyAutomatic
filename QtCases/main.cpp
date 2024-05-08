#include <QtPlugin>
#include <QtWidgets/qmessagebox.h>
#include "MainWindow.h"
#include <QApplication>
#include "WpsWidget.h"

// Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#ifdef _WIN32
    extern "C" __declspec(dllexport)
#endif

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w("flappy TRIANGLE");
    a.setActiveWindow(&w);
    w.show();
    WpsWidget wps;
    wps.showWidget();
    wps.test();
    return a.exec();
}
