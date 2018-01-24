//CVML
#ifdef WIN32
#include "../MfcUtil/opencv/CvimgMat.hpp"
#else
#ifdef _DEBUG
#error Only Release can work normally!
#endif
#include "../MfcUtil/tensor/CPyTensor.hpp"
#endif

int main()
{
#ifdef WIN32
	CvimgMat vm;
	vm.cvmat_test();
#else
	CPyTensor tf;
	tf.testPyfunc();
#endif
    return 0;
}
