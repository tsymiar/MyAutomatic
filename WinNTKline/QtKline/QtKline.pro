MYGL=../MFCKline/mygl
DEFINES += QT_VERSION
INCLUDEPATH += /usr/include/GL /usr/include/qt5 
INCLUDEPATH += ../MFCKline ../MFCKline/mygl ../MFCKline/font
HEADERS = MyOglWdg.h myuiobj.h #$${MYGL}/OGLKview.h
SOURCES = main.cpp MyOglWdg.cpp myuiobj.cpp #$${MYGL}/OGLKview.cc
QMAKE_CXXFLAGS += -std=c++0x
CONFIG += qt warn_on debug
QT += core gui
QT += opengl
QT += widgets
# greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
