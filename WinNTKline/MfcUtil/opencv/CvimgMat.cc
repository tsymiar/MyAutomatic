#include "CvimgMat.h"
#pragma warning (disable:4474)

using namespace cv;

Mat g_imageMat;

const cv::String g_cvPngStr =
#ifdef CVML
"../MfcUtil/image/taoxi.png"
#else
"../image/taoxi.png"
#endif
;

const cv::String g_cvJpgStr =
#ifdef CVML
"../MfcUtil/image/taoxi.jpg"
#else
"../image/taoxi.jpg"
#endif
;
struct MatImages {
    Mat rawImage;
    Mat mskImage;
};
struct ROIImages {
    MatImages roiImages;
    int flag = 0;
};

void createAlphaMat(Mat &mat)
{
    for (int i = 0; i < mat.rows; ++i)
    {
        for (int j = 0; j < mat.cols; ++j)
        {
            Vec4b&rgba = mat.at<Vec4b>(i, j);
            rgba[0] = UCHAR_MAX;
            rgba[1] = saturate_cast<uchar>((float(mat.cols - j)) / ((float)mat.cols) *UCHAR_MAX);
            rgba[2] = saturate_cast<uchar>((float(mat.rows - i)) / ((float)mat.rows) *UCHAR_MAX);
            rgba[3] = saturate_cast<uchar>(0.5 * (rgba[1] + rgba[2]));
        }
    }
}

static void onROIxyTrackBar(int fix, void* usrdata)
{
    ROIImages *images = (ROIImages*)(usrdata);
    // images->rawImage.flags < 0 ? (Mat*)memcpy(images, usrdata, sizeof(RoiImages)) : &imread(dstPng);
    if (images->roiImages.rawImage.flags <= 0) {
        Mat img = imread(g_cvPngStr);
        if (img.flags <= 0) {
            fprintf(stdout, "raw image is null!\n");
            return;
        }
        memcpy(&images->roiImages.rawImage, &img, sizeof(Mat));
    } else if (images->roiImages.mskImage.flags <= 0) {
        fprintf(stdout, "mask image is null!\n");
    }
    int x = 28;
    int y = 41;
    images->flag == 1 ? y = fix : x = fix;
    int w = images->roiImages.mskImage.cols;
    int h = images->roiImages.mskImage.rows;
    Mat imageROI = images->roiImages.rawImage(Rect(x, y, w, h));
    rectangle(images->roiImages.rawImage, Rect(x - 1, y - 1, w + 2, h + 2), Scalar(63, 63, 255), 2);
    addWeighted(imageROI, 0.2, images->roiImages.mskImage, 0.8, 0.0, imageROI);
    if (imageROI.data != NULL) {
        namedWindow("ROI", WINDOW_NORMAL);
        imshow("ROI", images->roiImages.rawImage);
    } else {
        fprintf(stdout, "addWeighted error, imageROI is null!\n");
    }
}

static void onBilateralTrackBar(int d, void* usrdata)
{
    Mat *src = (Mat*)(usrdata);
    Mat dst;
    // Mat& dst = image;
    if (src->flags < 0)
        src = &g_imageMat;
    bilateralFilter(*src, dst, d, d * 2, d / 2);
    imshow("Bilateral", dst);
}

static void onMedianFilterTrackBar(int ksize, void* usrdata)
{
    Mat src, dst;
    memcpy(&src, usrdata, sizeof(Mat));
    if (ksize == 3 || ksize == 5) {
        src = imread(g_cvJpgStr, CV_8U | CV_16U | CV_32F);
    }
    if (ksize > 40) {
        src = imread(g_cvJpgStr, CV_8U);
    }
    if (ksize % 2 == 0)
        ksize += 1;
    medianBlur(src, dst, ksize);
    imshow("MedianFilter", dst);
}

static void onThreshTrackBar(int old, void* usrdata)
{
    Mat dst, *src = (Mat*)(usrdata);
    // 二值化 
    if (src->flags < 0)
        src = &g_imageMat;
    threshold(*src, dst, old, 255, 0);
    imshow("Threshold", dst);
}

static void onMixedTrackBar(int value, void* usrdata)
{
    Mat mixImage;
    MatImages *images = (MatImages*)(usrdata);
    double alpha = value / 255.0;
    double beta = (1.0 - alpha);
    addWeighted(images->rawImage, beta, images->mskImage, alpha, 0.0, mixImage);
    imshow("Mixed Weights", mixImage);
}

int CvimgMat::saveMat2PNG(int w, int h, const String& name)
{
    Mat mat(w, h, CV_8UC4);
    createAlphaMat(mat);
    std::vector<int>compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
    try {
        imwrite(name, mat, compression_params);
    }
    catch (std::runtime_error& ex) {
        fprintf(stderr, "error trans jpeg to PNG: %s\n", ex.what());
        return -1;
    }
    fprintf(stdout, "save PNG file = '%s' (%d, %d).\n", name.c_str(), w, h);
    return 0;
}

Mat CvimgMat::getImageMat(const String& img, int flg)
{
    Mat dst, src = imread(img, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
    if (flg == 0)
        src = imread(img, 0);
    if (!src.data)
        fprintf(stdout, "error imread g_rawImage!\n");
    else
    {
        dst = src.clone();
        if (flg == -1)
        {
            namedWindow("Raw image", WINDOW_NORMAL);
            imshow("Raw image", src);
        }
    }
    return dst;
}

int CvimgMat::interestRegionImage(const String & src, const String & mask)
{
    saveMat2PNG(90, 90, g_cvPngStr);
    Mat logImage = imread(mask);

    if (!logImage.data)
    {
        fprintf(stdout, "error imread logImage!\n");
        return -1;
    }
    Mat imageROI{ Rect(0, 90, logImage.cols, logImage.rows) };
    Mat imageMask = imread(mask, CV_8U);
    //将掩膜拷贝到ROI   
    logImage.copyTo(imageROI, imageMask);
    imshow("imageROI[0,90]", imageROI);

    Mat srcImage = imread(src);
    if (!srcImage.data)
    {
        fprintf(stdout, "error imread roiImage!\n");
        return -1;
    }
    ROIImages *images = NULL;
    int len = sizeof(ROIImages);
    images = (ROIImages*)malloc(len);
    memset(images, 0, len);
    images->roiImages.rawImage = srcImage;
    images->roiImages.mskImage = logImage;
    images->flag = 0;
    int roival = 28;
    int xMax = srcImage.cols - logImage.cols;
    int yMax = srcImage.rows - logImage.rows;
    namedWindow("ROI", WINDOW_AUTOSIZE);
    createTrackbar("align X", "ROI", &roival, xMax, onROIxyTrackBar, images);
    ROIImages *imagesy = NULL;
    imagesy = (ROIImages*)malloc(len);
    memset(imagesy, 0, len);
    memcpy(imagesy, images, len);
    imagesy->flag = 1;
    createTrackbar("align Y", "ROI", &roival, yMax, onROIxyTrackBar, imagesy);
    imshow("ROI", srcImage);

    fprintf(stdout, "ROI mask logImage to imageMask OK.\n");
    return 0;
}

int CvimgMat::mixedModelImage(const String & img1, const String & img2)
{
    Mat image1 = imread(img1);
    saveMat2PNG(image1.rows, image1.cols, g_cvPngStr);
    Mat image2 = imread(img2);
    // size(dpi) of image must same as img2
    if (image1.cols != image2.cols || image2.rows != image1.rows) {
        fprintf(stdout, "g_mixImage: mismatch image's size!(%d,%d)->(%d,%d)\n", \
            image1.cols, image1.rows, image2.cols, image2.rows);
        return -2;
    }
    if (!image1.data || !image2.data) {
        fprintf(stdout, "error imread alpha Image!\n");
        return -1;
    }
    int iv = 0;
    MatImages *images = NULL;
    int len = 2 * sizeof(Mat);
    images = (MatImages*)malloc(len);
    memset(images, 0, len);
    images->rawImage = image1;
    images->mskImage = image2;
    namedWindow("Mixed Weights", 0);
    createTrackbar("weight", "Mixed Weights", &iv, 255, onMixedTrackBar, images);
    imshow("Mixed Weights", images->rawImage);
    fprintf(stdout, "mix img1 & img2 OK.\n");
    return 0;
}

int CvimgMat::bilateralImage(Mat srcImage)
{
    if (srcImage.flags <= 0) {
        fprintf(stdout, "bilateralImage srcImage flag invalid.\n");
        return -1;
    }
    int bilateralval = 8;
    namedWindow("Bilateral", 0);
    createTrackbar("bilateral", "Bilateral", &bilateralval, 400, onBilateralTrackBar, &srcImage);
    imshow("Bilateral", srcImage);
    fprintf(stdout, "BilateralFilter load OK.\n");
    return 0;
}

int CvimgMat::medianFilterImage(const String& image, int ksize)
{
    int len = sizeof(Mat);
    Mat srcImg = getImageMat(image);
    Mat* imgMat = (Mat*)malloc(len);
    memset(imgMat, 0, len);
    memcpy(imgMat, &srcImg, len);
    if (imgMat->flags <= 0) {
        fprintf(stdout, "medianFilterImage srcImage flag invalid.\n");
        return -1;
    }
    if (ksize % 2 == 0)
        ksize += 1;
    namedWindow("MedianFilter");
    createTrackbar("median", "MedianFilter", &ksize, 100, onMedianFilterTrackBar, imgMat);
    imshow("MedianFilter", srcImg);
    fprintf(stdout, "MedianFilter load OK.\n");
    return 0;
}

int CvimgMat::neighbourAverageImage(Mat srcImage, Size ksize)
{
    if (srcImage.flags <= 0) {
        fprintf(stdout, "neighbourAverageImage srcImage flag invalid.\n");
        return -1;
    }
    Mat avgimg;
    namedWindow("Neighbour Average");
    GaussianBlur(srcImage, avgimg, ksize, 0, 0);
    imshow("Neighbour Average", avgimg);
    fprintf(stdout, "GaussianFilter load OK.\n");
    return 0;
}

int CvimgMat::thresholdImage(Mat srcImage)
{
    if (srcImage.flags <= 0) {
        fprintf(stdout, "thresholdImage srcImage flag invalid.\n");
        return -1;
    }
    Mat imgDst;
    int posTrackBar = 200;
    cvtColor(srcImage, imgDst, CV_BGR2GRAY);
    namedWindow("Threshold");
    imshow("Threshold", srcImage);
    createTrackbar("thresh", "Threshold", &posTrackBar, 255, onThreshTrackBar, &srcImage);
    fprintf(stdout, "threshold for srcImage OK.\n");
    return 0;
}

int CvimgMat::cvmatTest()
{
    const cv::String srcImg =
#ifdef CVML
        "../MfcUtil/image/qdu.bmp"
#else
        "../image/qdu.bmp"
#endif
        ;
    interestRegionImage(srcImg, g_cvPngStr);
    mixedModelImage(g_cvJpgStr, g_cvPngStr);
    g_imageMat = getImageMat(g_cvJpgStr);
    bilateralImage(g_imageMat);
    medianFilterImage(g_cvJpgStr);
    neighbourAverageImage(g_imageMat);
    thresholdImage(g_imageMat);

    while (char(waitKey(1)) != 32) {}
    return 0;
}
