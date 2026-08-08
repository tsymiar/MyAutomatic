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
    
    // 背景滚动支持：加载 atlas 纹理并裁剪显示不同区域
    int loadAtlasBackground(const char* filename);
    void showBackground(float scrollU);
    bool isBgLoaded() const { return bgTexture != 0; }
    void setBgUWindow(float u) { bgUWindow = u; }
    void setBgVRange(float v) { bgVRange = v; }
    
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
    
    // 背景纹理滚动控制
    GLuint bgTexture = 0;
    float bgUWindow = 0.56f;  // 显示窗口占纹理宽度比例 (屏幕宽/纹理宽)
    float bgVRange = 0.5f;    // 背景在纹理高度方向的范围 (atlas 顶部背景区域占比)
};
