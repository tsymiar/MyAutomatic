#include "MainWindow.h"
#include <QtWidgets/qmessagebox.h>
#include <QApplication>
#include "OfficeWidget.h"
// #include <QtPlugin>
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
#ifdef SHOW_OFFICE
    OfficeWidget office;
    office.showWidget();
    office.test();
#endif
    return a.exec();
}
