#ifndef GLMODEL_H_
#define GLMODEL_H_
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include <cmath>
#include <cstdio>
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
#include "Def/MacroDef.h"
#include "mygl/GlF16.h"
#include "mygl/House.h"
#include "GL/glaux.h"
//链接器——》输入——》附加依赖项：legacy_studio_definitions.lib
#pragma comment(lib, "glaux.lib") 

class GlModel
{
public:
	void GlTexture(bool oo);
	int LoadGLTexture();
	void Load__QDU(int wide, int tall);
	void Model(int wide, int tall, float deltax = .0f, float deltay = .0f);
	void House(int wide, int tall);
};

class BMP
{
	friend class GlModel;
public:
	BMP(const char *FileName);
	virtual ~BMP();
	GLint getTex();
private:
	unsigned long horizon; //横
	unsigned long vertical;//竖
	char *Data = NULL;  //放置图像数据
	GLuint texture;
	void TexSet();
	bool Load(const char *filename);
};
#endif // !GLMODEL_H_
