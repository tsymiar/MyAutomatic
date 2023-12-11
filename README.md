<h1 align = "center">MyAutomatic</h1>

[![Build Status](https://tsymiar.visualstudio.com/MyAutomatic/_apis/build/status%2Ftsymiar.MyAutomatic?branchName=auto-dev)](https://tsymiar.visualstudio.com/MyAutomatic/_build/latest?definitionId=70&branchName=auto-dev)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://app.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

##### This is **MyAutomatic**,
```c
git clone https://github.com/tsymiar/MyAutomatic.git`
```
including sub-projects below ⇣⇣⇣

LinxSrvc
-------

* Building executes `./build.sh all -j` command. Testing using `./build.sh test`, deleting caches using `./build.sh clean`.

    Once when generates SUCCESS, some binary files will shown in the `bin`/`gen` directories, such as:

    *bin*
    ```c
    chstest
    chigpio
    mes909
    pipefifo
    VideoCapture
    imagesnap
    IM.exe
    Client.exe
    kaics.exe
    pthdtest.exe
    gSOAPverify(myweb.wsdl)
     ```
    *gen* *`gn webevent_server`*
    
    Descriptions:

    * 
       | chstest | chigpio | mes909 | pipefifo |
       | :------:| :--: | :----: | :-------:|

       Some scattered *.c files is driver of **hardware**s such as `GPIO`, `ME909S-821`(*a Huawei LTE 4G network module*), `pipe`/`fifo` etc.., *chstest* is a test to *chsdev* driver.


  * VideoCapture | imagesnap

      `VideoCapture` is a video capture program based on **v4l2** which should only able to run on linux.

      `imagesnap` is a photo take*r*, could running on linux only.

  * IM.exe | Client.exe

      [`IM.exe`](https://raw.githubusercontent.com/tsymiar/MyAutomatic/auto-dev/LinxSrvc/IM/IM.cc) is a simple `instant-messaging` chat room, use it by register, login, send command and *a small amount of quantity* messages.

      `Client.exe` is a client peer implement of the im chat room. ![IMClientDialog](WinNTKline/image/client.jpg)

  * kaics.exe

      a *sub-pub* message queue, which can penetrate the intranet, more info linked can get from [`here`](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/readme.md).

  * pthdtest.exe

      a thread pool based on `pthread`.

  * gSOAPverify

      a `SOAP-server` which is to verify login using the config file *myweb.wsdl*.

  * gn

      a *cross-platform*, *big/small endian*, *increasing/decreasing* binary number generator.

  * webevent_server

      a http server and client package library, depends on `libevent`.

QtCases
-------

* [it](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/QtCases) is a test-case uses Qt, OpenGL.
  
## WinNTKline:
> 
#####  [Microsoft .NET Framework 3.5](https://www.microsoft.com/en-US/download/details.aspx?id=25150) needed if compile WinNTKline

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

#### impact of the program in [`Market`]:

<img src="WinNTKline/image/impact.png" title="impact" height="60%" width="60%" align="middle"/>
