<h1 align = "center">MyAutomatic</h1>

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://www.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tsymiar/MyAutomatic&amp;utm_campaign=Badge_Grade) [![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

##### This is MyAutomatic, including subprojects below ⇣

LinxSrvc
-------
* Generats binary files by executing `make` into [gSOAPverify](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/LinxSrvc/gSOAPverify) directory.

    If makes OK, then try:
    ```c
    ./bin/gSOAPverify 8080
     ```
     to run it, it's a `SOAP-server` which is to verify login.
*  [IM](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/LinxSrvc/IM)
    could be compiled with `Visual Studio 2017` installed on Windows;
    
    also, run `g++ IM.cc -std=c++11 -lpthread` to compile if using linux/MacOS. [IM.cc](https://raw.githubusercontent.com/tsymiar/MyAutomatic/auto-dev/LinxSrvc/IM/IM.cc), is the source file.
*  hardware

   Some scattered .c files to driver hardwares such as `me909s`, GPIO, etc.

QtCases
-------
  #####  [QtCases](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/QtCases) is Qt with OpenGL.
  
## [WinNTKline](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/WinNTKline):
> 
######  [Microsoft .NET Framework 3.5](https://www.microsoft.com/en-US/download/details.aspx?id=25150) needed if compile WinNTKline

| CvMlwk |
|:----:|
```c
OpenCV && some Machine Learning learning cases.
``` 
| KlineUtil |
|:-------:|
```c
A MFC solution to register, catch log, show K-line, simulate ctp ... etc.
```
| WPFKline |
|:--------:|
```c
A K-line application using C#.
```
| TestUtils |
|:--------:|
>`testcases to test files of` _KlineUtil_ .

-------
#### impact of project `MFCKline`
![](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/WinNTKline/impact.png "IMPACT")
