#include "OglImgShow.h"
#include "SDL2tex.h"
#include <QDebug>
#include <QImage>
#include <QPen>

// 获取每一行所用的字节数，需凑足4的倍数
static int getRowBytes(int width)
{
    int bytes = 0;
    if ((width * 3) % 4 == 0) {
        bytes = width * 3;
    } else {
        bytes = ((width * 3) / 4 + 1) * 4;
    }
    return bytes;
}

OglImgShow::~OglImgShow()
{
    if (m_pixels != nullptr) {
        free(m_pixels);
        m_pixels = nullptr;
    }
    if (bgTexture != 0) {
        glDeleteTextures(1, &bgTexture);
        bgTexture = 0;
    }
}

int OglImgShow::setPixels(const char* filename)
{
    png_structp png_ptr;
    png_infop info_ptr;
    int bit_depth;
    FILE* fp = NULL;

    qDebug() << QString("lpng16( v%1 ), zlib( v%2 ), sdl( v%3 )").arg(PNG_LIBPNG_VER_STRING).arg(ZLIB_VERSION).arg(SDL_GetRevisionNumber());

    if (m_pixels != nullptr) {
        free(m_pixels);
        m_pixels = nullptr;
    }

    if (filename == NULL) {
        qFatal("image is null");
        return 0;
    }
    if ((fp = fopen(filename, "rb")) == NULL) {
        return EXIT_FAILURE;
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return EXIT_FAILURE;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return EXIT_FAILURE;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* Free all of the memory associated with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        /* If we get here, we had a problem reading the file */
        return EXIT_FAILURE;
    }
    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);
    png_get_IHDR(png_ptr, info_ptr, &m_width, &m_height, &bit_depth, &m_colour,
        NULL, NULL, NULL);
    qDebug() << "[" << m_width << "*" << m_height << "]";

    png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
    unsigned int size = 0;
    if (m_colour == PNG_COLOR_TYPE_RGB) {
        size = getRowBytes(m_width) * m_height;
    } else if (m_colour == PNG_COLOR_TYPE_RGBA) {
        size = m_width * m_height * 4;
    } else {
        // 不支持的色彩类型：释放 libpng 结构与文件句柄后返回
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return EXIT_FAILURE;
    }
    m_pixels = (unsigned char*)malloc(size);
    if (m_pixels == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return EXIT_FAILURE;
    }
    unsigned int i;
    for (i = 0; i < m_height; i++) {
        // 拷贝每行数据给pixel，
        // opengl原点在下方，拷贝时倒置
        if (m_colour == PNG_COLOR_TYPE_RGB) {
            memcpy(m_pixels + getRowBytes(m_width) * i, row_pointers[m_height - i - 1], m_width * 3);
        } else if (m_colour == PNG_COLOR_TYPE_RGBA) {
            memcpy(m_pixels + i * m_width * 4, row_pointers[m_height - i - 1], m_width * 4);
        }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(fp);
    return EXIT_SUCCESS;
}

GLuint OglImgShow::CreateTextureFromPng(const char* filename)
{
    unsigned char header[8];
    int k; // 循环计数器
    GLuint textureID; // 贴图ID
    int width, height; // 记录图片宽、高
    png_byte color_type; // 图片到类型（用于是否是开启来通道）
    png_byte bit_depth; // 字节深度

    png_structp png_ptr; // 图片
    png_infop info_ptr; // 图片信息
    int number_of_passes; // 隔行扫描
    png_bytep* row_pointers; // 图片数据内容
    int row, col, pos; // 图片像素排列
    GLubyte* rgba;

    if (filename == NULL) {
        qFatal("image is null");
        return 0;
    }
    FILE* fp = fopen(filename, "rb"); // 以只读形式打开文件
    if (NULL == fp) {
        printf("error: %s\n", strerror(errno)); // 关闭打开的文件给出默认贴图
        return 0;
    }
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        return 0;
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return 0;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        fclose(fp);
        return 0;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        // 释放占用内存（info_ptr 一并销毁）
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        fclose(fp);
        return 0;
    }
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        png_set_swap_alpha(png_ptr);
    }
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    number_of_passes = png_set_interlace_handling(png_ptr);
    qDebug() << "bit depth = " << bit_depth << ", passes number = " << number_of_passes << ".";
    png_read_update_info(png_ptr, info_ptr);
    if (setjmp(png_jmpbuf(png_ptr))) {
        // 读取失败：释放 libpng 结构与文件句柄
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 0;
    }
    rgba = (GLubyte*)malloc(width * height * 4);
    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    if (rgba == NULL || row_pointers == NULL) {
        free(rgba);
        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 0;
    }

    for (k = 0; k < height; k++)
        row_pointers[k] = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr,
            info_ptr));
    // 由于PNG像素是 左-右 顶-底 而贴图需要的像素都是 左-右 底-顶 所以需要把像素内容进行重新排列
    png_read_image(png_ptr, row_pointers);

    // 从最后一行开始写入，实现上下翻转（OpenGL 原点在左下）
    pos = 0;
    for (row = height - 1; row >= 0; row--) {
        for (col = 0; col < (4 * width); col += 4) {
            rgba[pos++] = row_pointers[row][col];        // red
            rgba[pos++] = row_pointers[row][col + 1];    // green
            rgba[pos++] = row_pointers[row][col + 2];    // blue
            rgba[pos++] = row_pointers[row][col + 3];    // alpha
        }
    }

    for (k = 0; k < height; k++)
        png_free(png_ptr, row_pointers[k]);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // GL_REPLACE 用纹理颜色替换片段颜色，配合 alpha 混合使透明像素真正透明
    //（GL_DECAL 对 RGBA 会忽略 alpha 导致透明区显示异常）
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    free(rgba);
    free(row_pointers);
    fclose(fp);
    return textureID;
}

void OglImgShow::loadGLTextures(const char* filename)
{
    if (filename != NULL) {
#ifdef QT_GUI_LIB
        QImage tex, img;
        if (!img.load(filename)) {
            qWarning("Could not read image file, using single-color instead.");
            QImage dummy(128, 128, QImage::Format_RGB32);
#ifdef QCOLOR_H
            dummy.fill(QColor("darkCyan").rgb());
#endif
            img = dummy;
            qDebug() << "image size = " << img.size();
        }
#ifdef QGL_H
        tex = QGLWidget::convertToGLFormat(img);
#endif
        // 重复调用前先删除旧纹理，避免 GPU 纹理累积泄漏
        if (texture[0] != 0)
            glDeleteTextures(3, texture);
        glGenTextures(2, &texture[0]);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, tex.width(), tex.height(), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());
        glBindTexture(GL_TEXTURE_2D, texture[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, tex.width(), tex.height(), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());
#endif
    } else {
        qFatal("image is null");
    }
}

void OglImgShow::showPngTexByName(const char* filename)
{
    glEnable(GL_TEXTURE_2D);
    if (0 == texture[0])
        texture[0] = CreateTextureFromPng(filename);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

int OglImgShow::loadAtlasBackground(const char* filename)
{
    bgTexture = CreateTextureFromPng(filename);
    if (bgTexture == 0) {
        qWarning("Failed to load background texture from: %s", filename);
        return EXIT_FAILURE;
    }
    // 设置纹理环绕为 CLAMP，避免边缘重复
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return EXIT_SUCCESS;
}

void OglImgShow::showBackground(float scrollU)
{
    if (bgTexture == 0) return;

    // 临时切换到正交投影，绘制全屏背景
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);  // 不写入深度缓冲区

    // 开启 alpha 混合，让纹理透明区域显示为清屏色（深色背景）
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 纹理 v 方向：显示绿色立柱带区域 [bgVOffset, bgVOffset+bgVRange]
    float v0 = bgVOffset;
    float v1 = bgVOffset + bgVRange;

    // 水平循环滚动：scrollU 在 [0, 1-bgUWindow] 之间，到达边界回绕
    float maxU = 1.0f - bgUWindow;
    if (maxU < 0.0f) maxU = 0.0f;
    float u = scrollU;
    if (maxU > 0.0f) {
        u = fmodf(scrollU, maxU);
        if (u < 0.0f) u += maxU;
    }
    float u0 = u;
    float u1 = u + bgUWindow;
    if (u1 > 1.0f) u1 = 1.0f;

    // 渲染全屏背景 quad，通过纹理坐标偏移实现裁剪+循环滚动效果
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);  glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(u1, v0);  glVertex2f(1.0f, -1.0f);
    glTexCoord2f(u1, v1);  glVertex2f(1.0f, 1.0f);
    glTexCoord2f(u0, v1);  glVertex2f(-1.0f, 1.0f);
    glEnd();

    glDepthMask(GL_TRUE);  // 恢复深度写入
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 显示图片
void OglImgShow::showPixels(png_uint_32 w, png_uint_32 h)
{
    glDisable(GL_DEPTH_TEST);
    // 是否设置图片透明度
    if (m_colour == PNG_COLOR_TYPE_RGB) {
        glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, m_pixels);
    } else if (m_colour == PNG_COLOR_TYPE_RGBA) {
        glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
    }
    glFlush();
    glEnable(GL_DEPTH_TEST);
}

void OglImgShow::showFullPixels()
{
    glDisable(GL_DEPTH_TEST);
    showPixels(m_width, m_height);
    glFlush();
    glEnable(GL_DEPTH_TEST);
}
