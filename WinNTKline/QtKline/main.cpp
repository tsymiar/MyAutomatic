#include "myuiobj.h"
#include <QtPlugin>
#include <QtWidgets\QApplication>
#include <QtWidgets\QMessagebox>
#include "MyOglWdg.h"

//Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

/*extern "C" __declspec(dllexport)*/int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QMyOglWdg w(0, 0);
	a.setActiveWindow(&w);
	w.show();
	return a.exec();
}
