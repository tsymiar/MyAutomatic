DEFINES += QT_DLL
INCLUDEPATH += /usr/include/GL
INCLUDEPATH += /usr/include/qt5
INCLUDEPATH += ../MFCKline
INCLUDEPATH += ../MFCKline/mygl
INCLUDEPATH += ../MFCKline/font
HEADERS = MyOglWdg.h myuiobj.h OGLKview.h
SOURCES = main.cpp MyOglWdg.cpp myuiobj.cpp OGLKview.cc
QMAKE_CXXFLAGS += -std=c++0x
CONFIG += qt warn_on debug
QT += core gui
QT += opengl
QT += widgets
# greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
