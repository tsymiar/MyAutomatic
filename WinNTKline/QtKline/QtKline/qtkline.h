#ifndef QTKLINE_H
#define QTKLINE_H

#include <QtWidgets/QMainWindow>
#include "ui_qtkline.h"

class QtKline : public QMainWindow
{
	Q_OBJECT

public:
	QtKline(QWidget *parent = 0);
	~QtKline();

private:
	Ui::QtKlineClass ui;
};

#endif // QTKLINE_H
