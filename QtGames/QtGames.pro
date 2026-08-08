# QT make file with [Qt5.12.8+/Creator4.11+]
CONFIG += qt warn_on debug \
        -finput-charset='UTF-8' \
        -fshort-wchar

QT += core gui network
QT += opengl
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += openglwidgets
    CONFIG += c++17
    QMAKE_CXXFLAGS += -std=c++17
} else {
    QMAKE_CXXFLAGS += -std=c++0x -Wno-attributes
}
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# ── check WPS SDK ──
unix {
    WPS_SDK = /opt/kingsoft/wps-office/office6
    greaterThan(QT_MAJOR_VERSION, 4) {
        WPS_LIB = $$WPS_SDK/librpcwpsapi_sysqt5.so
    } else {
        WPS_LIB = $$WPS_SDK/librpcwpsapi.so
    }
    exists($$WPS_LIB) {
        CONFIG += wps_support
    } else {
        warning("WPS SDK not found at $$WPS_SDK, compiling without WPS support")
    }
}

# ── WPS CONFIG only if wps_support ──
unix {
    contains(CONFIG, wps_support) {
        exists(/opt/kingsoft/wps-office/office6/libstdc++.so.6) {
            !exists(libstdc++.so.6) {
                system(ln -s /opt/kingsoft/wps-office/office6/libstdc++.so.6 libstdc++.so.6)
            }
            LIBS += /opt/kingsoft/wps-office/office6/libstdc++.so.6
        }

        QMAKE_LFLAGS += -Wl,--rpath=\'\$\$ORIGIN\':$$[QT_INSTALL_LIBS]:/opt/kingsoft/wps-office/office6
        QMAKE_LIBDIR =  ./ $$[QT_INSTALL_LIBS]  /opt/kingsoft/wps-office/office6

        greaterThan(QT_MAJOR_VERSION, 4) {
            LIBS += -lrpcwpsapi_sysqt5 -lrpcetapi_sysqt5 -lrpcwppapi_sysqt5
            exists(/opt/kingsoft/wps-office/office6/libc++abi.so.1) {
                !exists(libc++abi.so.1) {
                    system(ln -sf /opt/kingsoft/wps-office/office6/libc++abi.so.1 libc++abi.so.1)
                }
                LIBS += /opt/kingsoft/wps-office/office6/libc++abi.so.1
            }
        } else {
            LIBS += -lrpcwpsapi -lrpcetapi -lrpcwppapi
        }
    }
} else {
    QT += axcontainer
}

MYGL=external/kline/mygl
MYGL_ROOT=external/kline
# DEFINES += K_line # compile without K-line, to NOTE this line & HEADERS SOURCES include {MYGL}.
# DEFINES += _GLVBO_
unix:contains(CONFIG, wps_support) {
    DEFINES += SHOW_OFFICE
}

LIBS += -lglut \
        -lGLU \
        -lGL \
        -lpng \
        -lSDL2 \
        -lSDL2_image \
        -lSDL2_ttf

INCLUDEPATH += /usr/include/qt5 /usr/include/GL \
            $${MYGL_ROOT} $${MYGL} $${MYGL_ROOT}/font

# ── Only if wps_support ──
unix:contains(CONFIG, wps_support) {
    INCLUDEPATH += \
        third_party/wps_sdk/include/common \
        third_party/wps_sdk/include/wps \
        ./wpsapi
}

INCLUDEPATH += \
    app \
    engine \
    office

win32 {
    INCLUDEPATH += $$(libPNG) $$(ZLIB)
}

HEADERS = \
    app/MainWindow.h \
    engine/OglMaterial.h \
    engine/OglImgShow.h \
    external/kline/mygl/SDL2tex.h

# OGLKview.h 仅 K_line 宏定义时引入
contains(DEFINES, K_line) {
    HEADERS += external/kline/mygl/OGLKview.h
}

unix:contains(CONFIG, wps_support) {
    HEADERS += office/wpswindow.h \
            office/OfficeWidget.h
}

SOURCES = \
    app/main.cpp \
    app/MainWindow.cpp \
    engine/OglMaterial.cpp \
    engine/OglImgShow.cpp

contains(DEFINES, K_line) {
    SOURCES += external/kline/mygl/OGLKview.cc
}

unix:contains(CONFIG, wps_support) {
    SOURCES += office/wpswindow.cpp \
        office/OfficeWidget.cpp
}

unix {
    SOURCES += external/kline/mygl/SDL2tex.cc
}

FORMS += \
    resources/ui/mainwindow.ui

unix {
    RC_ICONS = resources/qtlogo.ico
    RESOURCES += \
        resources/qtlogo.qrc
}

DISTFILES +=
