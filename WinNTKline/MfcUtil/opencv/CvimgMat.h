#pragma once
#include <iostream>
#include <vector>
#ifdef REAL
#undef REAL
#endif
#include <opencv2/opencv.hpp>

struct CvimgMat {
    cv::Mat getImageMat(const cv::String& img, int flg = -1);
    int saveMat2PNG(int w, int h, const cv::String& name);
    int interestRegionImage(const cv::String& src, const cv::String& mask);
    int mixedModelImage(const cv::String& img1, const cv::String& img2);
    int bilateralImage(cv::Mat srcImage);
    int medianFilterImage(const cv::String& image, int ksize = 3);
    int neighbourAverageImage(cv::Mat srcImage, cv::Size ksize = cv::Size(3, 3));
    int thresholdImage(cv::Mat srcImage);
    int cvmatTest();
};
