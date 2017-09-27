#ifndef GLUIOBJ_H
#define GLUIOBJ_H

#include <QMainWindow>
#include "ui_openglwindow.h"

class GLUiObj : public QMainWindow
{
	Q_OBJECT

public:
	GLUiObj();
	~GLUiObj();

private:
	Ui::QtCasesClass ui;
};

#endif // MYUIOBJ_H
