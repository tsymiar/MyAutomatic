DEFINES += K_line #compile without K-line, to NOTE this line & HEADERS SOURCES include {MYGL}.
MYGL=../MFCKline/mygl
DEFINES += QT_VERSION
INCLUDEPATH += /usr/include/GL /usr/include/qt5 
INCLUDEPATH += ../MFCKline ../MFCKline/mygl ../MFCKline/font
HEADERS = OglMaterial.h OpenGLWindow.h gluiobj.h $${MYGL}/OGLKview.h $${MYGL}/SDL_text.h
SOURCES = main.cpp OpenGLWindow.cpp gluiobj.cpp OglMaterial.cpp $${MYGL}/OGLKview.cc $${MYGL}/SDL_text.cc
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
