/********************************************************************************
** Form generated from reading UI file 'myoglwdg.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OpenGLWindow_H
#define UI_OpenGLWindow_H

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
#include "OglMaterial.h"

QT_BEGIN_NAMESPACE

class Ui_QtCasesClass
{
public:
    QWidget *centralWidget;
    QOglMaterial *openGLWidget;
    QMenuBar *menuBar;
    QMenu *menu;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *QtCasesClass)
    {
        if (QtCasesClass->objectName().isEmpty())
            QtCasesClass->setObjectName(QStringLiteral("QtCasesClass"));
        QtCasesClass->resize(600, 400);
        QtCasesClass->setAutoFillBackground(false);
        centralWidget = new QWidget(QtCasesClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        openGLWidget = new QOglMaterial(centralWidget);
        openGLWidget->setObjectName(QStringLiteral("openGLWidget"));
        openGLWidget->setGeometry(QRect(0, 0, 600, 400));
        openGLWidget->setMaximumSize(QSize(600, 400));
        QtCasesClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(QtCasesClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 23));
        menu = new QMenu(menuBar);
        menu->setObjectName(QStringLiteral("menu"));
        QtCasesClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(QtCasesClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        QtCasesClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(QtCasesClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        QtCasesClass->setStatusBar(statusBar);

        menuBar->addAction(menu->menuAction());
        menu->addSeparator();
        mainToolBar->addSeparator();

        retranslateUi(QtCasesClass);

        QMetaObject::connectSlotsByName(QtCasesClass);
    } // setupUi

    void retranslateUi(QMainWindow *QtCasesClass)
    {
        QtCasesClass->setWindowTitle(QApplication::translate("QtCasesClass", "QtKline", 0));
        menu->setTitle(QApplication::translate("QtCasesClass", "Menu", 0));
    } // retranslateUi

};

namespace Ui {
    class QtCasesClass: public Ui_QtCasesClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OpenGLWindow_H
