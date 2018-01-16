//CVML
#ifdef WIN32
#include "../MfcUtil/opencv/CvimgMat.hpp"
#else
#include "../MfcUtil/py/CPyTensor.hpp"
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
