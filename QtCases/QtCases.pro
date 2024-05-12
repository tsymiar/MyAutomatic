CONFIG += qt warn_on debug \
        -finput-charset='UTF-8' \
        -fshort-wchar

QT += core gui network
QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QMAKE_CXXFLAGS += -std=c++0x -Wno-attributes

unix {
    exists(/opt/kingsoft/wps-office/office6/libstdc++.so.6) {
        system(ln -s /opt/kingsoft/wps-office/office6/libstdc++.so.6 libstdc++.so.6)
        LIBS += /opt/kingsoft/wps-office/office6/libstdc++.so.6
    }

    QMAKE_LFLAGS += -Wl,--rpath=\'\$\$ORIGIN\':$$[QT_INSTALL_LIBS]:/opt/kingsoft/wps-office/office6
    QMAKE_LIBDIR =  ./ $$[QT_INSTALL_LIBS]  /opt/kingsoft/wps-office/office6

    greaterThan(QT_MAJOR_VERSION, 4) {
        LIBS += -lrpcwpsapi_sysqt5 -lrpcetapi_sysqt5 -lrpcwppapi_sysqt5
        exists(/opt/kingsoft/wps-office/office6/libc++abi.so.1) {
            system(ln -sf /opt/kingsoft/wps-office/office6/libc++abi.so.1 libc++abi.so.1)
            LIBS += /opt/kingsoft/wps-office/office6/libc++abi.so.1
        }
    } else {
        LIBS += -lrpcwpsapi -lrpcetapi -lrpcwppapi
    }
} else {
    QT += axcontainer
}

# DEFINES += K_line # compile without K-line, to NOTE this line & HEADERS SOURCES include {MYGL}.
# DEFINES += _GLVBO_
MYGL=../WinNTKline/KlineUtil/mygl

LIBS += -lglut \
        -lGLU \
        -lGL \
        -lpng \
        -lSDL2 \
        -lSDL2_image \
        -lSDL2_ttf

INCLUDEPATH += /usr/include/qt5 /usr/include/GL \
            $${MYGL}/.. $${MYGL} $${MYGL}/../font

INCLUDEPATH += \
    include/common \
    include/wps \
    ./wpsapi

HEADERS = MainWindow.h OglMaterial.h OglImgShow.h \
    $${MYGL}/SDL2tex.h $${MYGL}/OGLKview.h \
    OfficeWidget.h

unix {
    HEADERS += wpsapi/wpswindow.h
} else {
}

SOURCES = main.cpp \
    MainWindow.cpp OglMaterial.cpp OglImgShow.cpp \
    $${MYGL}/SDL2tex.cc \  # $${MYGL}/OGLKview.cc \
    OfficeWidget.cpp

unix {
    SOURCES += wpsapi/wpswindow.cpp
} else {
}

FORMS += \
    mainwindow.ui

RC_ICONS = qtlogo.ico

RESOURCES += \
    qtlogo.qrc

DISTFILES +=
