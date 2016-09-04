#ifndef MYUIOBJ_H
#define MYUIOBJ_H

#include <QMainWindow>
#include "ui_myoglwdg.h"

class MyUiObj : public QMainWindow
{
	Q_OBJECT

public:
	MyUiObj();
	~MyUiObj();

private:
	Ui::QtKlineClass ui;
};

#endif // MYUIOBJ_H
