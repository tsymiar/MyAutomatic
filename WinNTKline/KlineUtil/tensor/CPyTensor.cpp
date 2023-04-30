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
	Py_SetProgramName(argv);
	PyObject* pyModule = PyImport_ImportModule((char*)"tensorflow");
	PyObject* pyFunc = PyObject_GetAttrString(pyModule, "Session");
	PyEval_CallObject(pyFunc, NULL);
	PyObject* pySess = PyObject_GetAttrString(pyFunc, "close"); 
	PyEval_CallObject(pySess, NULL);
	while (1) { ; };
	return 0;
}
