//CVML
#ifdef WIN32
#include "opencv/CvimgMat.h"
#include "tensor/ShannonEntropy.h"
#else
#ifdef _DEBUG
#error Only Release can work normally with x64/Python!
#endif
#include "../KlineUtil/tensor/CPyTensor.hpp"
#endif
int main()
{
#ifdef WIN32
    CvimgMat vm;
    //#undef CVML
    const cv::String img = "../KlineUtil/image/taoxi.jpg";
    cv::Mat mat = vm.getImageMat(img, 0);
    std::cout << "ShannonEntropy -> [" << img << "] = " << ShannonEntropy::getInstance()->SingleEntropy(mat) << std::endl;
    vm.cvMatTest(img);
#else
    CPyTensor tf;
    tf.testPyfunc();
#endif
    return 0;
}
