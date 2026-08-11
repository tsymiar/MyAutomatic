#include "MainWindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QCommandLineParser>
#ifdef SHOW_OFFICE
#include "OfficeWidget.h"
#endif
#ifdef ECHO_RESONANCE
#include "EchoMainWindow.h"
#endif
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

    QCommandLineParser parser;
    parser.setApplicationDescription("MyAutomatic QtGames Launcher");
    parser.addHelpOption();
#ifdef ECHO_RESONANCE
    parser.addOption({"echo", "启动余音回响 (Echo Resonance)"});
#endif
#ifdef SHOW_OFFICE
    parser.addOption({"office", "启动 WPS Office 集成"});
#endif
    parser.process(a);

#ifdef ECHO_RESONANCE
    if (parser.isSet("echo")) {
        EchoMainWindow echoWin;
        echoWin.show();
        return a.exec();
    }
#endif

#ifdef SHOW_OFFICE
    if (parser.isSet("office")) {
        OfficeWidget office;
        office.showWidget();
        office.showDoc();
        return a.exec();
    }
#endif

    // 默认：flappy TRIANGLE
    MainWindow w("flappy TRIANGLE");
    a.setActiveWindow(&w);
    w.setSdlWin();
    w.show();
    return a.exec();
}
