//CVML
#ifdef WIN32
#include "../MfcUtil/opencv/CvimgMat.h"
#include "../MfcUtil/tensor/ShannonEntropy.h"
#else
#ifdef _DEBUG
#error Only Release can work normally with x64/Python!
#endif
#include "../MfcUtil/tensor/CPyTensor.hpp"
#endif
int main()
{
#ifdef WIN32
	const cv::String img = "../MfcUtil/image/taoxi.png";
	CvimgMat vm;
//#undef CVML
#ifdef CVML
	vm.cvmat_test();
#endif
	cv::Mat mat = vm.getImageMat(img, 0);
	std::cout << "ShannonEntropy -> [" << img << "] = " << ShannonEntropy::getInstance()->SingleEntropy(mat) << std::endl;
	while (1);
#else
	CPyTensor tf;
	tf.testPyfunc();
#endif
    return 0;
}
