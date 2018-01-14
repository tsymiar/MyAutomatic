#include "CPyTensor.h"

CPyTensor::CPyTensor()
{
	Py_Initialize();
}

CPyTensor::~CPyTensor()
{
	Py_Finalize();
}

int CPyTensor::testPyfunc(wchar_t *argv)
{
	PyObject* pyModule = NULL;
	PyObject* pyFunc = NULL;
	PyObject* pySess = NULL;
	Py_SetProgramName(argv);
	pyModule = PyImport_ImportModule((char*)"tensorflow");
	pyFunc = PyObject_GetAttrString(pyModule, "Session");
	PyEval_CallObject(pyFunc, NULL);
	pySess = PyObject_GetAttrString(pyFunc, "close"); 
	PyEval_CallObject(pyFunc, NULL);
	while (1);
	return 0;
}
