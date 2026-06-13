cmake_minimum_required(VERSION 3.16)
project(QtGames VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt (Qt5 优先，回退 Qt6)
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL QUIET)
if(NOT Qt5_FOUND)
    find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets REQUIRED)
    set(QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets)
else()
    set(QT_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)
endif()

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)

pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_TTF   REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)

# GLUT (freeglut 无 .pc 文件，手动查找)
find_path(GLUT_INCLUDE_DIR GL/glut.h)
find_library(GLUT_LIBRARY glut)
if(GLUT_INCLUDE_DIR AND GLUT_LIBRARY)
    add_library(MyGLUT UNKNOWN IMPORTED)
    set_target_properties(MyGLUT PROPERTIES
        IMPORTED_LOCATION "${GLUT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLUT_INCLUDE_DIR}"
    )
    set(GLUT_INCLUDE_DIRS "${GLUT_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "GLUT (freeglut) not found. Install: sudo apt install freeglut3-dev")
endif()

# WPS Office 集成 (可选，需手动 -DENABLE_OFFICE=ON)
set(WPS_SDK_ROOT "/opt/kingsoft/wps-office/office6")
option(ENABLE_OFFICE "Enable WPS Office integration" OFF)
if(ENABLE_OFFICE AND EXISTS "${WPS_SDK_ROOT}")
    add_definitions(-DSHOW_OFFICE)
    message(STATUS "WPS Office integration: ENABLED")
else()
    message(STATUS "WPS Office integration: DISABLED (set -DENABLE_OFFICE=ON and install WPS SDK)")
endif()

set(APP_SOURCES
    app/main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
)

set(ENGINE_SOURCES
    engine/OglMaterial.cpp
    engine/OglMaterial.h
    engine/OglImgShow.cpp
    engine/OglImgShow.h
)

if(ENABLE_OFFICE AND EXISTS "${WPS_SDK_ROOT}")
    set(OFFICE_SOURCES
        office/OfficeWidget.cpp
        office/OfficeWidget.h
        office/wpswindow.cpp
        office/wpswindow.h
    )
else()
    set(OFFICE_SOURCES "")
endif()

set(EXTERNAL_SOURCES
    external/kline/mygl/SDL2tex.cc
    external/kline/mygl/SDL2tex.h
)

if(K_LINE)
    list(APPEND EXTERNAL_SOURCES
        external/kline/mygl/OGLKview.cc
        external/kline/mygl/OGLKview.h
    )
    add_definitions(-DK_line -DOGL_KVIEW_H_)
endif()

set(RESOURCE_FILES
    resources/ui/mainwindow.qrc
    resources/qtlogo.qrc
)

add_executable(QtGames
    ${APP_SOURCES}
    ${ENGINE_SOURCES}
    ${OFFICE_SOURCES}
    ${EXTERNAL_SOURCES}
    ${RESOURCE_FILES}
)

set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/external/kline/mygl
    ${CMAKE_CURRENT_SOURCE_DIR}/external/kline
    ${CMAKE_CURRENT_SOURCE_DIR}/external/kline/image
    ${OPENGL_INCLUDE_DIR}
    ${PNG_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

if(ENABLE_OFFICE AND EXISTS "${WPS_SDK_ROOT}")
    list(APPEND COMMON_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/office
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wps_sdk/include/common
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wps_sdk/include/wps
    )
endif()

target_include_directories(QtGames PRIVATE ${COMMON_INCLUDE_DIRS})

target_link_libraries(QtGames PRIVATE
    ${QT_LIBS}
    OpenGL::GL
    OpenGL::GLU
    PNG::PNG
    SDL2::SDL2
    PkgConfig::SDL2_IMAGE
    PkgConfig::SDL2_TTF
    PkgConfig::FREETYPE
    MyGLUT
)

# WPS 链接 (Linux)
if(ENABLE_OFFICE AND UNIX AND EXISTS "${WPS_SDK_ROOT}")
    target_link_directories(QtGames PRIVATE ${WPS_SDK_ROOT})
    target_link_libraries(QtGames PRIVATE
        rpcwpsapi_sysqt5
        rpcetapi_sysqt5
        rpcwppapi_sysqt5
    )
endif()

if(WIN32)
    target_link_libraries(QtGames PRIVATE Qt5::AxContainer dwmapi)
    if(DEFINED ENV{libPNG})
        target_include_directories(QtGames PRIVATE $ENV{libPNG})
    endif()
    if(DEFINED ENV{ZLIB})
        target_include_directories(QtGames PRIVATE $ENV{ZLIB})
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(QtGames PRIVATE -finput-charset=UTF-8)
endif()
