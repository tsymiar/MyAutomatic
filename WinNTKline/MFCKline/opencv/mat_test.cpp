#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

const cv::String bkgImg = "../image/atlas.png";
const cv::String testImg = "../image/timg.jpg";
const cv::String testPng = "../image/timg.png";

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

static void on_ROITrackBar(int alpha, void* usrdata)
{
	Mat dst, src = *(Mat*)(usrdata);
	addWeighted(src, alpha / 255.0, src, 1.0 - alpha / 255.0, 0.0, dst);
	imshow("ROI", dst);
}

static void on_MixTrackBar(int pos, void* usrdata)
{
	Mat dst, src = *(Mat*)(usrdata);
	// 二值化 
	threshold(src, dst, pos, 255, 0);
	imshow("Threshold", dst);
}

static void on_BilateralTrackBar(int pos, void* usrdata)
{
	Mat dst, src = *(Mat*)(usrdata);
	bilateralFilter(src, dst, pos, pos * 2, pos / 2);
	imshow("双边滤波", dst);
}

int main_test()
{
	int threshval = 16;
	int bilateralval = 10;
	Mat g_srcImage, Image1;
	g_srcImage = imread(testImg, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
	if (!g_srcImage.data)
	{
		printf("error imread g_srcImage！\n");
		return -1;
	}
	Image1 = g_srcImage.clone();

	namedWindow("原画", WINDOW_NORMAL);
	imshow("原画", g_srcImage);
	Mat mat(480, 480, CV_8UC4);
	createAlphaMat(mat);
	std::vector<int>compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);
	try {
		imwrite(testPng, mat, compression_params);
	}
	catch (std::runtime_error& ex) {
		fprintf(stderr, "error trans jpeg to PNG：%s\n", ex.what());
		return -1;
	}
	fprintf(stdout, "save alpha data of PNG.\n");

	Mat roiImage = imread(bkgImg);
	Mat logoImage = imread(testPng);
	if (!roiImage.data)
	{
		printf("error imread roiImage！\n");
		return -1;
	}
	if (!logoImage.data)
	{
		printf("error imread roiImage！\n");
		return -1;
	}
	Mat imageROI = roiImage(Rect(0, 200, logoImage.cols, logoImage.rows));
	Mat mask = imread(testPng, 0);
	//将掩膜拷贝到ROI   
	logoImage.copyTo(imageROI, mask);
	namedWindow("ROI", 0);
	imshow("ROI", roiImage);
	createTrackbar("image0", "ROI", &threshval, 255, on_ROITrackBar, &roiImage);
	fprintf(stdout, "ROI mask image0 to image1 OK.\n");

	double alphaValue = 0.3;
	double betaValue = (1.0 - alphaValue);
	Mat alphaImage, mixImage;
	alphaImage = imread(testPng);
	if (!alphaImage.data)
	{
		printf("error imread alphaImage！\n");
		return -1;
	}
	addWeighted(g_srcImage, alphaValue, alphaImage, betaValue, 0.0, mixImage);
	namedWindow("Threshold", 1);
	imshow("Threshold", g_srcImage);
	createTrackbar("threshold", "Threshold", &threshval, 255, on_MixTrackBar, &g_srcImage);
	fprintf(stdout, "threshold for g_srcImage OK.\n");

	namedWindow("混合加权", 0);
	imshow("混合加权", mixImage);
	fprintf(stdout, "mix image0 & image1 OK.\n");

	namedWindow("双边滤波", 0);
	createTrackbar("参数值：", "双边滤波", &bilateralval, 50, on_BilateralTrackBar, &g_srcImage);
	fprintf(stdout, "BilateralFilter load OK.\n");
/*
	Mat median, avgimg;
	medianBlur(g_srcImage, median, 3);
	imshow("中值滤波", median);
	fprintf(stdout, "MedianFilter load OK.\n");
	namedWindow("均值滤波");
	GaussianBlur(g_srcImage, avgimg, Size(3, 3), 0, 0);
	imshow("均值滤波", avgimg);
	fprintf(stdout, "GaussianFilter load OK.\n");
*/
	waitKey(60000);
	return 0;
}
