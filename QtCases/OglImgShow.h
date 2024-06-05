#pragma once
#include <cmath>
#include <cstdarg>
#include <iostream>
#include <QtOpenGL/QGL>
#include <QDebug>
#include <zlib.h>
#include <png.h>

class OglImgShow {
public:
    OglImgShow() { }
    ~OglImgShow() { }

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
};
