/********************************************************************************
** Form generated from reading UI file 'myoglwdg.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MYOGLWDG_H
#define UI_MYOGLWDG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include "MyOglWdg.h"

QT_BEGIN_NAMESPACE

class Ui_QtKlineClass
{
public:
    QWidget *centralWidget;
    QMyOglWdg *openGLWidget;
    QMenuBar *menuBar;
    QMenu *menu;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *QtKlineClass)
    {
        if (QtKlineClass->objectName().isEmpty())
            QtKlineClass->setObjectName(QStringLiteral("QtKlineClass"));
        QtKlineClass->resize(600, 400);
        QtKlineClass->setAutoFillBackground(false);
        centralWidget = new QWidget(QtKlineClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        openGLWidget = new QMyOglWdg(centralWidget);
        openGLWidget->setObjectName(QStringLiteral("openGLWidget"));
        openGLWidget->setGeometry(QRect(0, 0, 601, 381));
        openGLWidget->setMaximumSize(QSize(601, 381));
        QtKlineClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(QtKlineClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 23));
        menu = new QMenu(menuBar);
        menu->setObjectName(QStringLiteral("menu"));
        QtKlineClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(QtKlineClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        QtKlineClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(QtKlineClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        QtKlineClass->setStatusBar(statusBar);
        toolBar = new QToolBar(QtKlineClass);
        toolBar->setObjectName(QStringLiteral("toolBar"));
        QtKlineClass->addToolBar(Qt::TopToolBarArea, toolBar);

        menuBar->addAction(menu->menuAction());
        menu->addSeparator();
        mainToolBar->addSeparator();

        retranslateUi(QtKlineClass);

        QMetaObject::connectSlotsByName(QtKlineClass);
    } // setupUi

    void retranslateUi(QMainWindow *QtKlineClass)
    {
        QtKlineClass->setWindowTitle(QApplication::translate("QtKlineClass", "QtKline", 0));
        menu->setTitle(QApplication::translate("QtKlineClass", "\350\217\234\345\215\225", 0));
        toolBar->setWindowTitle(QApplication::translate("QtKlineClass", "toolBar", 0));
    } // retranslateUi

};

namespace Ui {
    class QtKlineClass: public Ui_QtKlineClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MYOGLWDG_H
