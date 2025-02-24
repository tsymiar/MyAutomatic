#if !defined(ALGORITHM_H)
#define ALGORITHM_H

#include <cmath>
#include <cfloat>
#include <cassert>
#include <random>
#ifdef _MSC_VER
#include <atlstr.h>
#endif
#define _N_ 10

namespace GlAlgorithm
{
    struct GLColour {
        float R;
        float G;
        float B;
        float A;
    };
    struct GLPoint {
        float x;
        float y;
    };
}

static GlAlgorithm::GLPoint /*CubicBézier*/CubicBezier(GlAlgorithm::GLPoint A[4], double t)
{
    GlAlgorithm::GLPoint P;

    double a1 = pow(1 - t, 3);
    double a2 = pow(1 - t, 2) * 3 * t;
    double a3 = 3 * t * t * (1 - t);
    double a4 = t * t * t;

    P.x = float(a1 * A[0].x + a2 * A[1].x + a3 * A[2].x + a4 * A[3].x);
    P.y = float(a1 * A[0].y + a2 * A[1].y + a3 * A[2].y + a4 * A[3].y);

    return P;
}
/*
   通过设置随机数引擎的种子
   在‘A’的基础上向后偏移
   构建一个从0到25之间的平均分布
   产生4个验证码字母字符
*/
static char* IdentifyCode(char* rtn)
{
    using namespace std;
    default_random_engine engine;
    uniform_int_distribution<int> uni_dist(0, 25);

    random_device rnd_device;
    engine.seed(rnd_device());

    const int bit = 4;
    char code[bit];
    for (int i = 0; i < bit; ++i) {
        code[i] = 'A' + uni_dist(engine);
    }
    code[bit - 1] = '\0';
    memcpy(rtn, code, bit);
    return rtn;
}

static int getDaysOfThisYear(int year, int month, int day, int* dest)
{
    if (year <= 0 || month <= 0 || month > 12 || day <= 0)
        return -1;
    int comm[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    int leap[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
    if (day > comm[month - 1] || day > leap[month - 1])
        return -1;
    int total = 0;
    for (int i = 0; i < month - 1; i++) {
        if (year % 400 || (year % 100 != 0 && year % 4 == 0))
            total += leap[i];
        else
            total += comm[i];
    }
    return *dest = month == 1 ? day : day + total;
}

struct BearCharges
{
    static double Sqrt_sum(int x, int y)
    {
        return sqrt(x * x + y * y);
    }

    static double Max_len(GlAlgorithm::GLPoint Base[_N_])
    {
        double S, max = 0;
        for (int i = 0; i < _N_; i++) {
            if (i != 0) {
                S = Sqrt_sum(int(Base[i].x - Base[i - 1].x), int(Base[i].y - Base[i - 1].y));
                if (max <= S)
                    break;
            }
        }
        return S;
    }
};

static void LeakCheck(void* ptr)
{
    if (ptr != nullptr) {
        delete (char*)ptr;
        ptr = nullptr;
    }
}
#endif
