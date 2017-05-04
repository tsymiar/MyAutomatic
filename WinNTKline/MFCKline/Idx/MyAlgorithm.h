#if !defined(ALGORITHM_H)
#define ALGORITHM_H

#include <cmath>
#include <mgl\OGLKview.h>

char* IdentifyCode(char* rtn)
{
	default_random_engine reng;
	// 构建一个从0到25之间的平均分布
	uniform_int_distribution<int>  uni_dist(0, 25);

	// 使用random_device设置随机数引擎的种子，
	// 以防止每次运行都产生相同的伪随机数序列
	random_device  rnd_device;
	reng.seed(rnd_device());

	// 验证码一共4位
	const int bit = 4;
	char code[bit]; // 保存验证码的字符数组
					// 利用for循环产生4个验证码字母字符
	for (int i = 0; i < bit; ++i)
	{
		// uni_dist(reng)表示让reng引擎按照uni_dist分布
		// 产生取值在0到25之间呈平均分布的随机数
		// 然后在‘A’的基础上向后偏移，就得到了随机的验证码字母字符
		code[i] = 'A' + uni_dist(reng);
	}
	code[bit] = '\0';
	memcpy(rtn, code, bit);
	return code;
}

class BearCharges
{
	double Sqrt_sum(int x, int y)
	{
		return sqrt(x*x + y*y);
	}

	double Max_len(OGLKview::Point Base[_N_]) {
		double S, max = 0;
		for (int i = 0; i < _N_; i++)
		{
			if (i != 0)
			{
				S = Sqrt_sum(int(Base[i].x - Base[i - 1].x), int(Base[i].y - Base[i - 1].y));
				if (max <= S)
					return S;
			}
			else
				return 0;
		}
	}
};
#endif
