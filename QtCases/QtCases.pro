# greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += qt warn_on debug \
        -finput-charset='UTF-8'
        -fshort-wchar
QT += core gui
QT += opengl
QT += widgets
QMAKE_CXXFLAGS += -std=c++0x

# DEFINES += K_line #compile without K-line, to NOTE this line & HEADERS SOURCES include {MYGL}.
# DEFINES += _GLVBO_
MYGL=../WinNTKline/KlineUtil/mygl
INCLUDEPATH += /usr/include/qt5 /usr/include/GL \
            $${MYGL}/.. $${MYGL} $${MYGL}/../font

LIBS += -lglut \
        -lGLU \
        -lGL \
        -lpng \
        -lSDL2 \
        -lSDL2_image \
        -lSDL2_ttf

HEADERS = MainWindow.h OglMaterial.h OglImgShow.h \
            $${MYGL}/SDL2tex.h $${MYGL}/OGLKview.h
SOURCES = main.cpp \
            MainWindow.cpp OglMaterial.cpp OglImgShow.cpp \
            $${MYGL}/SDL2tex.cc # $${MYGL}/OGLKview.cc

RC_ICONS = qtlogo.ico

RESOURCES += \
    qtlogo.qrc
