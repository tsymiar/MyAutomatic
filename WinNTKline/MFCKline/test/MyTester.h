#ifndef TEST_MINE_H_
#define TEST_MINE_H_
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <Set\MacroSets.h>
#include "ogl\Model.h"
#include "ogl\House.h"
#include "GL\glaux.h"
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
	unsigned long horizon; //ºá
	unsigned long vertical;//Êú
	char *Data;  //·ÅÖÃÍ¼ÏñÊý¾Ý
	bool Load(char *filename);
	GLuint texture;
	void TexSet();
	BMP(char *FileName);
};
#endif // !TEST_MINE_H_

