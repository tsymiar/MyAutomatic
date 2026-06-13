#pragma once
#include <cmath>
#include <cstdarg>
#include <iostream>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLFunctions>
#else
#include <QtOpenGL/QGL>
#endif
#include <zlib.h>
#include <png.h>

class OglImgShow {
public:
    OglImgShow() {}
    ~OglImgShow();

    int setPixels(const char* filename);
    void showFullPixels();
    void showPixels(png_uint_32 width, png_uint_32 height);

    void showPngTexByName(const char* filename);
private:
    GLuint CreateTextureFromPng(const char* filename);
    void loadGLTextures(const char* filename);
private:
    GLuint texture[3] = { 0, 0, 0 };
    const char* m_filename = NULL;
    unsigned char* m_pixels = nullptr;
    png_uint_32 m_width = 0;
    png_uint_32 m_height = 0;
    int m_colour = 0;
};
