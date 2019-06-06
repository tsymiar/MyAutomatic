#pragma once
#include <iostream>
#include <vector>
#ifdef REAL
#undef REAL
#endif
#include <opencv2/opencv.hpp>

struct MatImgSet {
    cv::String name;
    cv::Mat image;
};
class CvimgMat {
private:
    MatImgSet* getImageSet(const cv::String& img, const cv::String& name);
public:
    cv::Mat getImageMat(const cv::String& img, int flg = -1);
    int interestRegionImage(const cv::String& src, const cv::String& mask);
    int mixedModelImage(const cv::String& img1, const cv::String& img2);
    int medianFilterImage(const cv::String& src, int value = 3);
    int neighbourAverageImage(const cv::String& src, cv::Size ksize = cv::Size(3, 3));
    int bilateralImage(const cv::String & src);
    int thresholdImage(const cv::String& src);
    int cvmatTest(const cv::String& file);
};
