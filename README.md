<h1 align = "center">MyAutomatic</h1>

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://www.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tsymiar/MyAutomatic&amp;utm_campaign=Badge_Grade) [![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

##### This is MyAutomatic, including sub-projects below ⇣

LinxSrvc
-------
* Generats by executing `./build.sh -j` command.

    Once When build SUCCESS, some binary files will shown in `bin` directory, such as:
    ```c
    gpio
    me909
    tmpfifo
    VideoCapture
    snap
    IM.exe
    Client.exe
    kaics.exe
    pthdtest.exe
    httpsample
    gSOAPverify
    myweb.wsdl
     ```
    * gpio me909 tmpfifo

      Some scattered .c files is driver of **hardware**s such as `GPIO`, `me909s`(*a 4G network module*), `fifo`.

    * VideoCapture snap
       
      `VideoCapture` is a video capture program based on **v4l2** which should only running on linux.

      `snap` is a photo take*r*, only runs on linux.

  * IM.exe Client.exe

      [`IM.exe`](https://raw.githubusercontent.com/tsymiar/MyAutomatic/auto-dev/LinxSrvc/IM/IM.cc) is a simple `instant-messaging` chat room, use it by register, login, send command and *a small amount of quantity* messages.

      `Client.exe` is a client peer of the chat room.

  * kaics.exe

      a *sub-pub* message queue, which can penetrate the intranet, more info linked can get from [`here`](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/readme.md).

  * pthdtest.exe

      a thread pool based on `pthread`.

  * httpsample

      a http server and client package library, depends on `libevent`.

  * gSOAPverify

      a `SOAP-server` which is to verify login using the config file *myweb.wsdl*.

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

#### impact of the `Market`:

<img src="WinNTKline/impact.png" title="impact" height="50%" width="50%">
