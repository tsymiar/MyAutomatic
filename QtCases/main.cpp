#include <QtPlugin>
#include <QtWidgets/qmessagebox.h>
#include "MainWindow.h"
#include <QApplication>
#include "OfficeWidget.h"

// Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#ifdef _WIN32
#ifdef main
#undef main
#endif
    extern "C" __declspec(dllexport)
#endif

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w("flappy TRIANGLE");
    a.setActiveWindow(&w);
    w.setSdlWin();
    w.show();
    OfficeWidget office;
    office.showWidget();
    office.test();
    return a.exec();
}
