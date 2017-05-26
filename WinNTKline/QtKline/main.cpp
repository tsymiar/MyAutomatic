#include "myuiobj.h"
#include <QtPlugin>
#include <QtWidgets\QApplication>
#include <QtWidgets\QMessagebox>
#include "MyOglWdg.h"

//Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

extern "C" __declspec(dllexport)int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	volatile bool fs = false;
	switch (QMessageBox::information(0,
		"Start FullScreen?",
		"Would You Like To Run In Fullscreen Mode?",
		QMessageBox::Yes,
		QMessageBox::No | QMessageBox::Default))
	{
	case QMessageBox::Yes:
		fs = true;
		break;
	case QMessageBox::No:
		fs = false;
		break;
	}
	QMyOglWdg w(0, 0, fs);
	a.setActiveWindow(&w);
	w.show();
	return a.exec();
}
