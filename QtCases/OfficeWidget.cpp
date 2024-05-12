#include "OfficeWidget.h"
#include <QCloseEvent>
#include <QDebug>

OfficeWidget::OfficeWidget(QWidget *)
{
}

void OfficeWidget::showWidget()
{
    initApp();
    resize(480, 640);
    show();
}

OfficeWidget::~OfficeWidget()
{
    deleteLater();
}

void OfficeWidget::test()
{
    openDoc("./wpsapi/file/testword.docx");
    // printOutDoc();
    qDebug() << ("OfficeWidget::getContent=") << getDocContent();
}

void OfficeWidget::closeEvent(QCloseEvent * event)
{
    if (event->type() == QEvent::Type::Close) {
        closeDoc();
    }
#ifdef WIN32
    QWidget::closeEvent(event);
#else
    WPSMainWindow::closeEvent(event);
#endif
}

#ifdef WIN32
void OfficeWidget::initApp()
{

}
void OfficeWidget::openDoc(const char* file)
{
    
}
QString OfficeWidget::getDocContent()
{
    return {};
}
void OfficeWidget::closeDoc()
{

}
#endif
