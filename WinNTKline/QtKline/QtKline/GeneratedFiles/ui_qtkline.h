/********************************************************************************
** Form generated from reading UI file 'qtkline.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTKLINE_H
#define UI_QTKLINE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QOpenGLWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QtKlineClass
{
public:
    QWidget *centralWidget;
    QOpenGLWidget *openGLWidget;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *QtKlineClass)
    {
        if (QtKlineClass->objectName().isEmpty())
            QtKlineClass->setObjectName(QStringLiteral("QtKlineClass"));
        QtKlineClass->resize(600, 400);
        QtKlineClass->setAutoFillBackground(false);
        centralWidget = new QWidget(QtKlineClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        openGLWidget = new QOpenGLWidget(centralWidget);
        openGLWidget->setObjectName(QStringLiteral("openGLWidget"));
        openGLWidget->setGeometry(QRect(0, 0, 601, 381));
        QtKlineClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(QtKlineClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 23));
        QtKlineClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(QtKlineClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        QtKlineClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(QtKlineClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        QtKlineClass->setStatusBar(statusBar);

        mainToolBar->addSeparator();

        retranslateUi(QtKlineClass);

        QMetaObject::connectSlotsByName(QtKlineClass);
    } // setupUi

    void retranslateUi(QMainWindow *QtKlineClass)
    {
        QtKlineClass->setWindowTitle(QApplication::translate("QtKlineClass", "QtKline", 0));
    } // retranslateUi

};

namespace Ui {
    class QtKlineClass: public Ui_QtKlineClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QTKLINE_H
