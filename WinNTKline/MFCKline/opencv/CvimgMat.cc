#include "CvimgMat.h"

#pragma warning (disable:4474)
using namespace cv;

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
	Mat dst, src1 = *(Mat*)(usrdata);
	addWeighted(src1, alpha / 255.0, src1, 1.0 - alpha / 255.0, 0.0, dst);
	imshow("ROI", dst);
}

static void on_ThreshTrackBar(int old, void* usrdata)
{
	Mat dst, src = *(Mat*)(usrdata);
	// 二值化 
	threshold(src, dst, old, 255, 0);
	imshow("Threshold", dst);
}

static void on_BilateralTrackBar(int d, void* usrdata)
{
	Mat dst, src = *(Mat*)(usrdata);
	bilateralFilter(src, dst, d, d * 2, d / 2);
	imshow("Bilateral", dst);
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
		fprintf(stderr, "error trans jpeg to PNG：%s\n", ex.what());
		return -1;
	}
	fprintf(stdout, "save alpha data of PNG.\n");
	return 0;
}

Mat CvimgMat::g_srcImage(const String& img)
{
	Mat g_srcImage, Image1;
	g_srcImage = imread(img, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
	if (!g_srcImage.data)
		printf("error imread g_srcImage！\n");
	else
	{
		Image1 = g_srcImage.clone();
		namedWindow("g_srcImage", WINDOW_NORMAL);
		imshow("g_srcImage", g_srcImage);
	}
	return Image1;
}

int CvimgMat::g_roiImage(const String & bkg, const String & mask, int defaultval)
{
	Mat roiImage = imread(bkg);
	Mat logImage = imread(mask);
	if (!roiImage.data)
	{
		printf("error imread roiImage！\n");
		return -1;
	}
	if (!logImage.data)
	{
		printf("error imread roiImage！\n");
		return -1;
	}
	Mat imageROI = roiImage(Rect(0, 200, logImage.cols, logImage.rows));
	Mat imageMask = imread(mask, 0);
	//将掩膜拷贝到ROI   
	logImage.copyTo(imageROI, imageMask);
	namedWindow("ROI", 0);
	imshow("ROI", roiImage);
	createTrackbar("image:", "ROI", &defaultval, 255, on_ROITrackBar, &roiImage);
	fprintf(stdout, "ROI mask logImage to imageMask OK.\n");
	return 0;
}

int CvimgMat::g_limImage(Mat imgSrc, const String alphapng, int threshold)
{
	double alphaVal = 0.3;
	double betaVal = (1.0 - alphaVal);
	Mat alphaImage, limImage;
	alphaImage = imread(alphapng);
	if (!alphaImage.data)
	{
		printf("error imread alphaImage！\n");
		return -1;
	}
	addWeighted(imgSrc, alphaVal, alphaImage, betaVal, 0.0, limImage);
	namedWindow("Threshold", 1);
	imshow("Threshold", imgSrc);
	createTrackbar("thresh:", "Threshold", &threshold, 255, on_ThreshTrackBar, &imgSrc);
	fprintf(stdout, "threshold for g_srcImage OK.\n");
	return 0;
}

int CvimgMat::g_mixImage(const String & img1, const String & img2, double alpha)
{
	Mat mixImage;
	Mat image1 = imread(img1);
	Mat image2 = imread(img2);
	namedWindow("Mixed Weights", 0);
	addWeighted(image1, alpha, image2, 1.0 - alpha, 0.0, mixImage);
	imshow("Mixed Weights", mixImage);
	fprintf(stdout, "mix img1 & img2 OK.\n");
	return 0;
}

int CvimgMat::g_bilImage(Mat imgSrc, int bilateralval)
{
	namedWindow("Bilateral", 0);
	createTrackbar("val:", "Bilateral", &bilateralval, 50, on_BilateralTrackBar, &imgSrc);
	fprintf(stdout, "BilateralFilter load OK.\n");
	return 0;
}

int CvimgMat::g_medImage(Mat imgSrc, int ksize)
{
	Mat median, avgimg;
	medianBlur(imgSrc, median, ksize);
	imshow("MedianFilter", median);
	fprintf(stdout, "MedianFilter load OK.\n");
	return 0;
}

int CvimgMat::g_aveImage(Mat imgSrc, Size ksize)
{
	Mat avgimg;
	namedWindow("Neighbour Average");
	GaussianBlur(imgSrc, avgimg, ksize, 0, 0);
	imshow("Neighbour Average", avgimg);
	fprintf(stdout, "GaussianFilter load OK.\n");
	return 0;
}

int CvimgMat::cvmat_test()
{
	const cv::String bkgImg = "../image/qdu.bmp";
	const cv::String srcJpg = "../image/timg.jpg";
	const cv::String dstPng = "../image/timg.png";

	Mat image=g_srcImage(srcJpg);
	saveMat2PNG(480, 480, dstPng);

	g_roiImage(bkgImg, dstPng);
	g_limImage(image, dstPng);
	g_mixImage(srcJpg, dstPng, 0.5);
	g_bilImage(image, 8);
	g_medImage(image, 3);
	g_aveImage(image);

	waitKey(60000);
	return 0;
}
