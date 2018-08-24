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
    int g_roiImage(const cv::String& bkg, const cv::String& mask, int threshval = 8);
    int g_mixImage(const cv::String& img1, const cv::String& img2, double alpha);
    int g_limImage(cv::Mat imgSrc, const cv::String alphapng);
    int g_bilImage(cv::Mat imgSrc, int& bilateralval);
    int g_medImage(cv::Mat imgSrc, int ksize = 3);
    int g_aveImage(cv::Mat imgSrc, cv::Size ksize = cv::Size(3, 3));
    int g_eshImage(cv::Mat imgSrc);
    int cvmat_test();
};
