#include "wpswidget.h"

WpsWidget::WpsWidget(QWidget *parent) : QWidget(parent)
{
    m_wps = new WPSMainWindow(this);
}

void WpsWidget::showWidget()
{
    m_wps->initApp();
    m_wps->setGeometry(QRect(0, 0, 800, 400));
    m_wps->show();
}

WpsWidget::~WpsWidget()
{
    m_wps->deleteLater();
}

void WpsWidget::test()
{
    m_wps->openDoc("./wps/file/testword.docx");
}
