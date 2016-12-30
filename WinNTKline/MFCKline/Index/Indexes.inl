#ifndef _INDEXS_H
#define _INDEXS_H

#include<cmath>
#include<cfloat>
#include<assert.h>
#include<atlstr.h>
#include<gl/GL.h>

class Indexes {

public:
	Indexes::Indexes(){}

	struct GLColor {
		GLfloat R;
		GLfloat G;
		GLfloat B;
		GLfloat A;
	};
	struct GLPoint {
		GLfloat x;
		GLfloat y;
	};
	GLPoint CubicB¨¦zier(GLPoint A[4], double t)
	{
		GLPoint P;

		double a1 = pow(1 - t, 3);
		double a2 = pow(1 - t, 2) * 3 * t;
		double a3 = 3 * t*t*(1 - t);
		double a4 = t*t*t;

		P.x = float(a1*A[0].x + a2*A[1].x + a3*A[2].x + a4*A[3].x);
		P.y = float(a1*A[0].y + a2*A[1].y + a3*A[2].y + a4*A[3].y);

		return P;
	}
private:
	char* ptr;
	LONG len;
public:
	static char* Indexes::AllocBuffer(CString msg)
	{
		Indexes index;
#ifdef _UNICODE
		index.len = WideCharToMultiByte(CP_ACP, 0, msg, -1, NULL, 0, NULL, NULL);
		index.ptr = new char[index.len + 1];
		memset(index.ptr, 0, index.len + 1);
		WideCharToMultiByte(CP_ACP, 0, msg, -1, index.ptr, index.len + 1, NULL, NULL);
#else
		index.ptr = new char[msg.GetAllocLength() + 1];
		sprintf(index.ptr, _T("%s"), (LPSTR)(LPCTSTR)msg);
#endif // _UNICODE
		char* p = index.ptr;
		assert(index.ptr != NULL);
		return p;
	}
public:
	virtual Indexes::~Indexes()
	{
		if (ptr)
			ptr = NULL;
	}
};
#endif