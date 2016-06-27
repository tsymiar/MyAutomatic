#ifndef MODEL_H
#define MODEL_H
#include <math.h>
#include <stdio.h>
#include "texture.h"

#ifdef __cplusplus
extern "C"
{
	extern float _dX, _dY;
	void AdjustModel();
	void BuildF16(int wide, int tall);
}
#endif
#endif // !MODEL_H
