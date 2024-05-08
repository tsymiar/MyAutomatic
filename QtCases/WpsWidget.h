#ifndef WPSWIDGET_H
#define WPSWIDGET_H

#include <QWidget>
#include "wpswindow.h"

class WpsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WpsWidget(QWidget *parent = 0);
    ~WpsWidget();

signals:

public slots:
private:
    WPSMainWindow* m_wps = NULL;
public:
    void showWidget();
    void test();
};

#endif // WPSWIDGET_H
