#DEFINES += K_line #compile without K-line, to NOTE this line & HEADERS SOURCES include {MYGL}.
MYGL=../WinNTKline/KlineUtil/mygl
DEFINES += QT_VERSION
INCLUDEPATH += /usr/include/GL /usr/include/qt5 \
            $${MYGL}/.. $${MYGL} $${MYGL}/../font
QMAKE_CXXFLAGS += -std=c++0x
LIBS += -lglut \
        -lpng \
        -lGLU \
        -lGL \
        -lSDL2_image \
        -lSDL2_ttf
CONFIG += qt warn_on debug \
        -finput-charset='UTF-8'
        -fshort-wchar
QT += core gui
QT += opengl
QT += widgets
# greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

HEADERS = $${MYGL}/SDL_text.h \
          OglMaterial.h OpenGLWindow.h OglImage.h
          # $${MYGL}/OGLKview.h
SOURCES = $${MYGL}/SDL_text.cc \
          main.cpp OglImage.cpp OpenGLWindow.cpp OglMaterial.cpp
          # $${MYGL}/OGLKview.cc
