#include "CvimgMat.h"
#pragma warning (disable:4474)
using namespace cv;
Mat image;
const cv::String dstPng =
#ifdef CVML
"../MfcUtil/image/taoxi.png"
#else
"../image/taoxi.png"
#endif
;

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

static void on_ROITrackBar(int weight, void* usrdata)
{
	Mat dst, *src1 = NULL;
	int len = sizeof(Mat);
	src1 = (Mat*)malloc(len);
	memset(src1, 0, len);
	src1 = src1->data ? (Mat*)memcpy(src1, usrdata, len) : &imread(dstPng);
	if (!src1->data)
		printf("data error in on_ROITrackBar(src1 is null)!\n");
	else
	{
		addWeighted(*src1, weight / 255.0, *src1, 1.0 - (weight / 255.0), 0.0, dst);
		imshow("ROI", dst);
	}
}

static void on_BilateralTrackBar(int d, void* usrdata)
{
	Mat dst, *src = (Mat*)(usrdata);
	if (src->flags < 0)
		src = &image;
	bilateralFilter(*src, dst, d, d * 2, d / 2);
	imshow("Bilateral", dst);
}

static void on_ThreshTrackBar(int old, void* usrdata)
{
	Mat dst, *src = (Mat*)(usrdata);
	// 二值化 
	if (src->flags < 0)
		src = &image;
	threshold(*src, dst, old, 255, 0);
	imshow("Threshold", dst);
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
	fprintf(stdout, "save alpha data of PNG.\n");
	return 0;
}

Mat CvimgMat::getImageMat(const String& img, int flg)
{
	Mat dst, src = imread(img, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
	if (flg == 0)
		src = imread(img, 0);
	if (!src.data)
		printf("error imread g_rawImage!\n");
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

int CvimgMat::g_roiImage(const String & bkg, const String & mask, int defaultval)
{
	Mat roiImage = imread(bkg);
	Mat logImage = imread(mask);
	if (!roiImage.data)
	{
		printf("error imread roiImage!\n");
		return -1;
	}
	if (!logImage.data)
	{
		printf("error imread logImage!\n");
		return -1;
	}
	Mat imageROI = roiImage(Rect(0, 100, logImage.cols, logImage.rows));
	Mat imageMask = imread(mask, 0);
	//将掩膜拷贝到ROI   
	logImage.copyTo(imageROI, imageMask);
	namedWindow("ROI", 0);
	imshow("ROI", roiImage);
	createTrackbar("image:", "ROI", &defaultval, 255, on_ROITrackBar, &roiImage);
	fprintf(stdout, "ROI mask logImage to imageMask OK.\n");
	return 0;
}

// size(dpi) of image must same as img2
int CvimgMat::g_mixImage(const String & img1, const String & img2, double alpha)
{
	Mat mixImage;
	Mat image1 = imread(img1);
	Mat image2 = imread(img2);
	if (image1.cols != image2.cols || image2.rows != image1.rows)
	{
		printf("g_mixImage: mismatch image's size!(%d,%d)->(%d,%d)\n", \
			image1.cols, image1.rows, image2.cols, image2.rows);
		return -2;
	}
	namedWindow("Mixed Weights", 0);
	addWeighted(image1, alpha, image2, 1.0 - alpha, 0.0, mixImage);
	imshow("Mixed Weights", mixImage);
	fprintf(stdout, "mix img1 & img2 OK.\n");
	return 0;
}

int CvimgMat::g_limImage(Mat imgSrc, const String alphapng)
{
	double alphaVal = 0.3;
	double betaVal = (1.0 - alphaVal);
	Mat alphaImage, limImage;
	alphaImage = imread(alphapng);
	if (alphaImage.cols != imgSrc.cols || imgSrc.rows != alphaImage.rows)
	{
		printf("g_limImage: mismatch image's size!(%d,%d)->(%d,%d)\n", \
			alphaImage.cols, alphaImage.rows, imgSrc.cols, imgSrc.rows);
		return -2;
	}
	if (!alphaImage.data)
	{
		printf("error imread alphaImage!\n");
		return -1;
	}
	addWeighted(imgSrc, alphaVal, alphaImage, betaVal, 0.0, limImage);
	imshow("limImage", limImage);
	return 0;
}

int CvimgMat::g_bilImage(Mat imgSrc, int& bilateralval)
{
	namedWindow("Bilateral", 0);
	imshow("Bilateral", imgSrc);
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

int CvimgMat::g_eshImage(Mat imgSrc)
{
	Mat imgDst;
	int posTrackBar = 16;
	cvtColor(imgSrc, imgDst, CV_BGR2GRAY);
	namedWindow("Threshold");
	imshow("Threshold", imgSrc);
	createTrackbar("thresh:", "Threshold", &posTrackBar, 255, on_ThreshTrackBar, &imgSrc);
	fprintf(stdout, "threshold for g_rawImage OK.\n");
	return 0;
}

int CvimgMat::cvmat_test()
{
	const cv::String bkgImg =
#ifdef CVML
		"../MfcUtil/image/qdu.bmp"
#else
		"../image/qdu.bmp"
#endif
		;
	const cv::String srcJpg =
#ifdef CVML
		"../MfcUtil/image/taoxi.jpg"
#else
		"../image/taoxi.jpg"
#endif
		;

	image = getImageMat(srcJpg);
	saveMat2PNG(90, 90, dstPng);
	int bilval = 8;
	int weight = 64;
	float alpha = 0.5f;
	g_roiImage(bkgImg, dstPng, weight);
	g_mixImage(srcJpg, dstPng, alpha);
	g_limImage(image, dstPng);
	g_bilImage(image, bilval);
	g_medImage(image);
	g_aveImage(image);
	g_eshImage(image);

	waitKey(60000);
	return 0;
}
