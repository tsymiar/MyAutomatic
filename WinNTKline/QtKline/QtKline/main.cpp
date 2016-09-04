#include "myuiobj.h"
#include <QtWidgets/QApplication>

extern "C" __declspec(dllexport)int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MyUiObj w;
	w.show();
	return a.exec();
}
