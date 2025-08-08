from setuptools import setup, Extension

module = Extension('sudoku',
                  sources=['extend.c', 'sudoku.c'])

setup(name='sudoku',
      version='1.0',
      description='Python extension sudoku',
      ext_modules=[module])
