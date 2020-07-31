#pragma once
#include <cmath>
#include <cstdarg>
#include <iostream>
#include <QtOpenGL/QGL>
#include <QDebug>
#include <zlib.h>
#include <png.h>

class OglImage
{
public:
    OglImage() {};
    ~OglImage() {};
    int setPixels(const char* filename);
    void Show();
    void Show(const char* filename);
private:
    GLuint texture[3] = { NULL };
private:
    void loadGLTextures(const char* filename);
    GLuint CreateTextureFromPng(const char* filename);
};
