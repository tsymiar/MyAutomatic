#ifndef STOCK_CLASS_H_
#define STOCK_CLASS_H_

class Stock {
public:	Stock() : alph(0.f), EMA12(0.f), EMA26(0.f), DIF(0.f), f_dea{ } {}
        //destructor
        ~Stock() {}
        //variables
private:
    float alph;
    float EMA12, EMA26;
    float DIF, f_dea[9];
    //structs
public:
    struct Ema {
        float yema12;
        float yema26;
    };
    struct Sma {
        int X;
        int M;
        int N;
        struct MA {
            float _5;
            float _10;
            float _20;
        };
    };
    struct Rsi {
        int t_y;
        int t_m;
        int t_d;
        float open;
        float high;
        float low;
        float close;
        int volume;
        int price;
        struct RSI {
            float _6;
            float _12;
            float _24;
        } stRSA;
    };
    //functions
public:
    float RSI(float A, float B)
    {
        return (A + B) == 0 ? 0.0f : A / (A + B);
    }
    float SMA(Sma ma, float pY)
    {
        if (ma.N <= ma.M)
            return 0;
        else
            // 转 float 避免整数乘法溢出
            return ((float)ma.M * (float)ma.X + (float)(ma.N - ma.M) * pY) / (float)ma.N;
    }
    float EMA(float EMA_y, float toclose, int N)
    {
        alph = 2 / ((float)N + 1);
        return alph * toclose + (1 - alph)*EMA_y;
    }
    // dea 已退化为指针，长度必须显式传入
    float DEA(float dea[], int size = 9)
    {
        if (dea == nullptr || size <= 0)
            return 0.f;
        float sum = 0;
        for (int i = 0; i < size; i++)
            sum += dea[i];
        return sum / size;
    }
    float MACD(Ema ema, float yesDEA, float toclose)
    {
        EMA12 = ema.yema12 * 11 / 13 + toclose * 2 / 13;
        EMA26 = ema.yema26 * 25 / 27 + toclose * 2 / 27;
        DIF = EMA12 - EMA26;
        float macd = ((DIF - DEA(f_dea)) * 2) + yesDEA;
        return macd;
    }
    float MACD(float DIF, float DEA)
    {
        return (DIF - DEA) * 2;
    }
};
#endif // !STOCK_CLASS_H_
