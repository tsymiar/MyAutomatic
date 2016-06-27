#include "qtkline.h"
#include <QtWidgets/QApplication>

extern "C" __declspec(dllexport)int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QtKline w;
	w.show();
	return a.exec();
}
