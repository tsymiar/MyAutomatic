#ifndef OFFICE_WIDGET_H
#define OFFICE_WIDGET_H

#include <QWidget>
#ifdef WIN32

class OfficeWidget : public QWidget
{
#else
#include "wpswindow.h"

class OfficeWidget : public WPSMainWindow
{
    Q_OBJECT
#endif
public:
    explicit OfficeWidget(QWidget *parent = 0);
    ~OfficeWidget();

signals:

public slots:

public:
    void closeEvent(QCloseEvent *);

    void showWidget();
    void test();

private:
#ifdef WIN32
    void initApp();
    void openDoc(const char* file);
    QString getDocContent();
    void closeDoc();
#endif
};

#endif // OFFICE_WIDGET_H
