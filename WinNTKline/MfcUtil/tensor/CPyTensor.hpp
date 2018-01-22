#pragma once
// PYTHONPATH/Lib/argparse.py--1622
// prog = _os.path.basename(_sys.argv[0])
// _sys.argv[0]=>>__file__
#include <Python.h>  

class CPyTensor
{
public:
	CPyTensor(); 
	virtual ~CPyTensor();
	int testPyfunc(wchar_t *argv = (wchar_t*)"CPyTensor");
};

#include "CPyTensor.cpp"
