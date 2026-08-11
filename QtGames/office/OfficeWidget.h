#ifndef OFFICE_WIDGET_H
#define OFFICE_WIDGET_H

#include <QWidget>
#ifdef _WIN32
#include <QAxObject>

class OfficeWidget : public QWidget {
#else
#include "wpswindow.h"

class OfficeWidget : public WPSMainWindow {
    Q_OBJECT
#endif
public:
    explicit OfficeWidget(QWidget* parent = 0);
    ~OfficeWidget();

signals:

public slots:

public:
    void closeEvent(QCloseEvent*);

    void showWidget();
    void showDoc(std::string file = "./wpsapi/file/testword.docx");

private:
#ifdef _WIN32
    void initDocApp();
    void openDoc(const char* file);
    QString getDocContent();
    void closeDoc();
private:
    QAxObject* m_axCom = NULL;
    QAxObject* m_doc = NULL;
#endif
};

#endif // OFFICE_WIDGET_H
