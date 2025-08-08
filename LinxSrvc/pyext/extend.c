#include <Python.h>

extern void sudoku();

static PyObject* game(PyObject* self, PyObject* args)
{
    sudoku();
    fprintf(stdout, "%s", "Backtrace finished");
    return Py_None;
}

static PyMethodDef SudokuMethods[] = {
    {"game", game, METH_VARARGS, "Sudoku function"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef sudoku_module = {
    PyModuleDef_HEAD_INIT,
    "sudoku",
    NULL,
    -1,
    SudokuMethods
};

PyMODINIT_FUNC PyInit_sudoku(void)
{
    return PyModule_Create(&sudoku_module);
}
