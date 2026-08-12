#include "MainWindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
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
    // 未传参数时打印启动方法
    const QString usage = QString(argv[0]) + " [参数]\n"
        "  --flappy\t启动 flappy TRIANGLE\n"
#ifdef ECHO_RESONANCE
        "  --echo\t启动「余音回响」(Echo Resonance)\n"
#endif
#ifdef SHOW_OFFICE
        "  --office\t启动 WPS Office 集成\n"
#endif
        "  --help\t帮助";
    if (argc < 2) std::cout << "Usage:\t" << qPrintable(usage) << std::endl;

    QApplication a(argc, argv);

    // 向上查找包含 external/kline/image 目录
    {
        QDir dir(QCoreApplication::applicationDirPath());
        QString resRel = "external/kline/image";
        // 从可执行文件目录向上最多查找 3 层
        for (int i = 0; i < 3; ++i) {
            if (dir.exists(resRel)) {
                QDir::setCurrent(dir.absolutePath());
                break;
            }
            if (!dir.cdUp()) break;
        }
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("MyAutomatic QtGames Launcher");
    parser.addHelpOption();
    parser.addOption({ "flappy", "启动 flappy TRIANGLE" });
#ifdef ECHO_RESONANCE
    parser.addOption({ "echo", "启动 余音回响 (Echo Resonance)" });
#endif
#ifdef SHOW_OFFICE
    parser.addOption({ "office", "启动 WPS Office 集成" });
#endif
    parser.process(a);

    bool launched = false;

    if (parser.isSet("flappy")) {
        MainWindow* w = new MainWindow("flappy TRIANGLE");
        a.setActiveWindow(w);
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setSdlWin();
        w->show();
        launched = true;
    }
#ifdef ECHO_RESONANCE
    if (parser.isSet("echo")) {
        EchoMainWindow* echoWin = new EchoMainWindow;
        echoWin->setAttribute(Qt::WA_DeleteOnClose);
        echoWin->show();
        launched = true;
    }
#endif
#ifdef SHOW_OFFICE
    if (parser.isSet("office")) {
        OfficeWidget* office = new OfficeWidget;
        office->setAttribute(Qt::WA_DeleteOnClose);
        office->showWidget();
        office->showDoc();
        launched = true;
    }
#endif

    if (launched)
        return a.exec();

    QMessageBox::information(nullptr, "QtGames 启动方法：", usage);
    return 0;
}
