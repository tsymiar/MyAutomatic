#DEFINES += K_line #compile without K-line, to NOTE this line & HEADERS SOURCES include {MYGL}.
#MYGL=../MfcUtil/mygl
DEFINES += QT_VERSION
INCLUDEPATH += /usr/include/GL /usr/include/qt5
#INCLUDEPATH += ../MfcUtil ../MfcUtil/mygl ../MfcUtil/font
HEADERS = OglMaterial.h OpenGLWindow.h OglImage.h #$${MYGL}/OGLKview.h $${MYGL}/SDL_text.h
SOURCES = main.cpp OglImage.cpp OpenGLWindow.cpp OglMaterial.cpp #$${MYGL}/OGLKview.cc $${MYGL}/SDL_text.cc
QMAKE_CXXFLAGS += -std=c++0x
LIBS += -lSDL_image \
        -lSDL_ttf \
        -lglut \
        -lpng \
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
