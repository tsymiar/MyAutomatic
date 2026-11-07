#include "MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#ifdef SHOW_OFFICE
#include "OfficeWidget.h"
#endif
#ifdef ECHO_RESONANCE
#include "EchoMainWindow.h"
#endif

#ifdef _WIN32
#ifdef main
#undef main
#endif
extern "C" __declspec(dllexport)
#endif

int main(int argc, char* argv[])
{
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

    // 模式互斥：一次只启动一个窗口，取第一个命中的选项
    bool launched = false;
    if (parser.isSet("flappy")) {
        MainWindow* w = new MainWindow("flappy TRIANGLE");
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setSdlWin();
        w->show();
        w->activateWindow();
        launched = true;
    }
#ifdef ECHO_RESONANCE
    else if (parser.isSet("echo")) {
        EchoMainWindow* echoWin = new EchoMainWindow;
        echoWin->setAttribute(Qt::WA_DeleteOnClose);
        echoWin->show();
        launched = true;
    }
#endif
#ifdef SHOW_OFFICE
    else if (parser.isSet("office")) {
        OfficeWidget* office = new OfficeWidget;
        office->setAttribute(Qt::WA_DeleteOnClose);
        office->showWidget();
        office->showDoc();
        launched = true;
    }
#endif

    if (launched)
        return a.exec();

    // 无匹配模式：打印用法（与 --help 输出同源，避免两处维护）
    parser.showHelp();
    return 0;
}
