<h1 align = "center">MyAutomatic</h1>

[![Build Status](https://tsymiar.visualstudio.com/MyAutomatic/_apis/build/status%2Ftsymiar.MyAutomatic?branchName=auto-dev)](https://tsymiar.visualstudio.com/MyAutomatic/_build/latest?definitionId=70&branchName=auto-dev)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://app.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

##### This is **_`MyAutomatic`_**, clone with:
```c
git clone https://github.com/tsymiar/MyAutomatic.git
```
##### _includes sub-projects below ⇣⇣⇣_

LinxSrvc
-------

* Brief

    Building executes `./build.sh all -j` command. Testing using `./build.sh test`, deleting caches using `./build.sh clean`.

    Once when generates _SUCCESS_, some binary files will shown in the _bin_ / _gen_ directories, such as:

    [_bin_]
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
    [_gen_]
    
     _`gn` / `webevent_server`_
    
* Describe

    + 
       | chstest | chigpio | mes909 | pipefifo |
       | :------:| :--: | :----: | :-------:|

       Some scattered _`*.c`_ files is driver of **hardware**s such as `GPIO`, `ME909S-821`(*a Huawei `LTE 4G` network module*), `pipe`/`fifo` _etc._; *chstest* is a test to *chsdev* driver.


  + VideoCapture | imagesnap

      _VideoCapture_ is a video capture program based on **v4l2** which should *only* able to run on linux.

      _imagesnap_ is a photo take*r*, could running on linux *only*.

  + IM.exe | Client.exe

      [_`IM.exe`_](https://raw.githubusercontent.com/tsymiar/MyAutomatic/auto-dev/LinxSrvc/IM/IM.cc) is a simple `instant-messaging` chat room, use it by register, login, send command and *a small amount of quantity* messages.

      `Client.exe` is a client peer implement of the im chat room. ![IMClientDialog](WinNTKline/image/client.jpg)

  + kaics.exe

      a *sub-pub* message queue(_`MQ`_), which can penetrate the intranet, more info linked can get from [*here*](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/readme.md).

  + pthdtest.exe

      a thread pool based on `pthread`.

  + gSOAPverify

      a `SOAP-server` which is to verify login using the config file *myweb.wsdl*.

  + gn

      a *cross-platform*, *big/small endian*, *increasing/decreasing* binary number generator.

  + webevent_server

      a http server and client package library, depends on `libevent`.

QtCases
-------

* [_`It`_](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/QtCases) is a test-case using _`Qt`_, _`OpenGL`_.
  
## WinNTKline:

#####  [Microsoft .NET Framework 3.5](https://www.microsoft.com/en-US/download/details.aspx?id=25150) needed if compile WinNTKline

| CvMlwk |
|:----:|

> _`OpenCV`_ && some _`Machine Learning`_ learning cases.

| KlineUtil |
|:-------:|

> A MFC solution to *register*, catch *log*, show *K-line*, simulate *ctp* ... _etc._

| WPFKline |
|:--------:|

> A K-line application using _`C#`_.

| TestUtils |
|:--------:|
> A testcases to test files of _KlineUtil_ .

-------

#### _**I**mpact of the program in [`Market`]:_

<img src="WinNTKline/image/impact.png" title="impact" height="60%" width="60%" align="middle"/>