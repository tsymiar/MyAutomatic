#pragma once
#include <cmath>  
#include <cstdarg>
#include <iostream> 
#include <QtOpenGL/QGL>
#include <QDebug>
#include<../zlib-1.2.8/zlib.h>
#include <png.h>

class ShowImage
{
public:
	ShowImage();
	~ShowImage();
	int setPixels(const char* filename);
	void Show();
	void Show(const char* filename);
private:
	GLuint texture[3] = { NULL };
private:
	void loadGLTextures(const char* filename);
	GLuint CreateTextureFromPng(const char* filename);
};

