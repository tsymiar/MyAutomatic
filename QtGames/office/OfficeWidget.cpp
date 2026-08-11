#include "OfficeWidget.h"
#include <QCloseEvent>
#include <QDebug>

OfficeWidget::OfficeWidget(QWidget* parent)
#ifdef _WIN32
    : QWidget(parent)
#else
    : WPSMainWindow(parent)
#endif
{}

void OfficeWidget::showWidget()
{
    initWps();
    resize(480, 640);
    show();
}

OfficeWidget::~OfficeWidget()
{}

void OfficeWidget::showDoc(std::string file)
{
    openDoc(file.c_str());
    // printOutDoc();
    qDebug() << ("OfficeWidget::getContent=") << getDocContent();
}

void OfficeWidget::closeEvent(QCloseEvent* event)
{
    if (event->type() == QEvent::Type::Close) {
        closeDoc();
    }
#ifdef _WIN32
    QWidget::closeEvent(event);
#else
    WPSMainWindow::closeEvent(event);
#endif
}

#ifdef _WIN32
void OfficeWidget::initDocApp()
{
    m_axCom = new QAxObject("Word.Application");
    m_axCom->dynamicCall("SetVisible(bool Visible)", "false");
    m_axCom->setProperty("DisplayAlerts", false);
}

void OfficeWidget::openDoc(const char* file)
{
    QAxObject* docs = m_axCom->querySubObject("Documents");
    if (docs != NULL) {
        docs->dynamicCall("Open(const QVariant&)", QVariant(QString(file)));
        m_doc = m_axCom->querySubObject("ActiveDocument");
    }
}

QString OfficeWidget::getDocContent()
{
    QString content = "";
    if (m_doc != NULL) {
        QAxObject* select = m_doc->querySubObject("Range()");
        content = select->property("Text").toString();
    }
    return content;
}

void OfficeWidget::closeDoc()
{
    if (m_doc != NULL) {
        m_doc->dynamicCall("Close()");
        delete m_doc;
        m_doc = NULL;
    }
    if (m_axCom != NULL) {
        m_axCom->dynamicCall("Quit()");
        delete m_axCom;
        m_axCom = NULL;
    }
}
#endif
