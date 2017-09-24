DEFINES += K_line 
MYGL=../MFCKline/mygl
DEFINES += QT_VERSION
INCLUDEPATH += /usr/include/GL /usr/include/qt5 
INCLUDEPATH += ../MFCKline ../MFCKline/mygl ../MFCKline/font
HEADERS = Material.h MyOglWdg.h myuiobj.h $${MYGL}/OGLKview.h $${MYGL}/SDL_text.h
SOURCES = main.cpp MyOglWdg.cpp myuiobj.cpp Material.cpp $${MYGL}/OGLKview.cc $${MYGL}/SDL_text.c
QMAKE_CXXFLAGS += -std=c++0x
LIBS += -lSDL_image \
	-lSDL_ttf \
	-lglut \
	-lGLU \
	-lGL \
	-lSDL 
CONFIG += qt warn_on debug \
	-finput-charset='UTF-8' 
	-fshort-wchar
QT += core gui
QT += opengl
QT += widgets
# greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
