#pragma once
#include <opencv2/opencv.hpp>

class ShannonEntropy
{
private:
	ShannonEntropy() {};
	ShannonEntropy(const ShannonEntropy&);
	static ShannonEntropy *m_entropy;
	ShannonEntropy& operator=(const ShannonEntropy&) {};
	~ShannonEntropy() {};
public:
	static ShannonEntropy* getInstance();
	// 单幅图像信息熵
	// 公式：H = -\sum_{ x\epsilon U }{P(x)\log P(x)}
	// 1. 遍历整个图像矩阵，获取每一个像素灰度值
	// 2. 求得每个灰度值在图像矩阵中出现的概率
	// 3. 根据一定的算法，统计灰度值的总期望
	double SingleEntropy(cv::Mat img);
	// 两幅图像联合信息熵
	double CommonEntropy(cv::Mat img1, cv::Mat img2, double img1_entropy, double img2_entropy);
};
