<h1 align = "center">MyAutomatic</h1>

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/c4a1d9328fef4a9099cb262db7705cb3)](https://app.codacy.com/gh/tsymiar/MyAutomatic?utm_source=github.com&utm_medium=referral&utm_content=tsymiar/MyAutomatic&utm_campaign=Badge_Grade)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://www.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tsymiar/MyAutomatic) [![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

##### This is **MyAutomatic** [`git clone https://github.com/tsymiar/MyAutomatic.git`], including sub-projects below ⇣

LinxSrvc
-------
* Generats by executing `./build.sh all -j` command.

    Once when build SUCCESS, some binary files will shown in the `bin/gen` directory, such as:

    *gen*
    ```c
    gn
    webevent_server
     ```
    *bin*
    ```c
    chstest
    chigpio
    mes909
    pipefifo
    VideoCapture
    imagesnap
    Client.exe
    kaics.exe
    pthdtest.exe
    gSOAPverify/myweb.wsdl
     ```
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

  * webevent_server

      a http server and client package library, depends on `libevent`.

  * gSOAPverify

      a `SOAP-server` which is to verify login using the config file *myweb.wsdl*.

  * gn

      a *cross-platform*, *big/small endian*, *increasing/decreasing* binary number generator.

QtCases
-------

* [it](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/QtCases) is a test with Qt, OpenGL.
  
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

#### impact of the exe in `Market`:

<img src="WinNTKline/image/impact.png" title="impact" height="50%" width="50%" align="middle"/>
