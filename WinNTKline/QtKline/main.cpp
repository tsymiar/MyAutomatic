#include "myuiobj.h"
#include <QtPlugin>
#include <QtWidgets/qmessagebox.h>
#include "MyOglWdg.h"

// Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#ifdef _WIN32
extern "C" __declspec(dllexport)
#endif
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QMyOglWdg w(0, 0);
	a.setActiveWindow(&w);
	w.show();
	return a.exec();
}
