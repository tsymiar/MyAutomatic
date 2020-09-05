#ifndef GLF16_H
#define GLF16_H
#include <cmath>
#include <cstdio>
#include "texture.h"

#ifdef __cplusplus
extern "C"
{
#endif
    extern float _dX, _dY;
    void AdjustModel();
    void BuildF16(int wide, int tall);
#ifdef __cplusplus
}
#endif
#endif // !GLF16_H
