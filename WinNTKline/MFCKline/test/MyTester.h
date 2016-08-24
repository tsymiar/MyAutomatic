#ifndef TEST_MINE_H_
#define TEST_MINE_H_
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "Def\MacroDef.h"
#include "ogl\Model.h"
#include "ogl\House.h"
#include "GL\glaux.h"
//链接器——》输入——》附加依赖项：legacy_studio_definitions.lib
#pragma comment(lib, "GL\\glaux.lib") 

class CMyTester
{
public:
	CMyTester();
	virtual ~CMyTester();
	void testbegin();
	void testend();
	void glTEST(bool oo);
	int LoadGLTexture();
	void Load__qdu(int wide, int tall);
	void Model(int wide, int tall, float deltax, float deltay);
	void House(int wide, int tall);
	//ClassName* pObj;
};
class BMP
{
public:
	unsigned long horizon; //横
	unsigned long vertical;//竖
	char *Data;  //放置图像数据
	bool Load(char *filename);
	GLuint texture;
	void TexSet();
	BMP(char *FileName);
};
#endif // !TEST_MINE_H_

