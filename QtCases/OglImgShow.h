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
    void showPixels();

    void ShowPngTex(const char* filename);
private:
    GLuint CreateTextureFromPng(const char* filename);
    void loadGLTextures(const char* filename);
private:
    GLuint texture[3] = { 0, 0, 0 };
};
