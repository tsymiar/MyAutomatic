<h1 align = "center">MyAutomatic</h1>

[![CMake on multiple platforms](https://github.com/tsymiar/MyAutomatic/actions/workflows/cmake-multi-platform.yml/badge.svg?branch=auto-dev)](https://github.com/tsymiar/MyAutomatic/actions/workflows/cmake-multi-platform.yml)
[![Build Status](https://tsymiar.visualstudio.com/MyAutomatic/_apis/build/status%2Ftsymiar.MyAutomatic?branchName=auto-dev)](https://tsymiar.visualstudio.com/MyAutomatic/_build/latest?definitionId=70&branchName=auto-dev)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/af21f03e75a14429a74a0ec437d41993)](https://app.codacy.com/gh/tsymiar/MyAutomatic/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

##### This is **_`MyAutomatic`_**, getting by

```c
git clone https://github.com/tsymiar/MyAutomatic.git
```

##### _includes sub-projects below ⇣⇣⇣_

LinxSrvc
-------

* Brief

    Building all executes by `./build.sh all -j8` command. Using `./build.sh test` to test, deleting caches use `./build.sh clean`.

    Once when generates _SUCCESS_, some binary files will shown in the _bin_ / _gen_ directories, such as:

    [_bin_]

    ```c
    chstest chigpio mes909 pipefifo
    VideoCapture imagesnap
    IM.exe client.exe
    kaics.exe
    gSOAPverify(myweb.wsdl)
    pthdtest.exe
     ```

    [_gen_]

     _`gn1` / `webevent_server` / `dpsk_chat` / `analyzing`_

* Description

  *
       | chstest | chigpio | mes909 | pipefifo |
       | :------:| :--: | :----: | :-------:|

       Some scattered _`*.c`_ files is driver of **hardware**s such as `GPIO`, `ME909S-821`(_a Huawei `LTE 4G` network module_), `pipe`/`fifo` _etc._; _chstest_ is a sample to _chsdev_ driver.

  * VideoCapture | imagesnap

      _VideoCapture_ is a video capture program based on **v4l2** which should _only_ able to run on linux.

      _imagesnap_ is a photo take*r*, could running on linux _only_.

  * IM.exe | client.exe

      [_`IM.exe`_](https://raw.githubusercontent.com/tsymiar/MyAutomatic/auto-dev/LinxSrvc/IM/IM.cc) is a `instant-messaging` chat room demo, use it by register, login, send command and _a small amount of quantity_ messages.

      `client.exe` is a client peer implement of an _online chat room_ menus like below.

      <img src="WinNTKline/image/client.jpg" title="IMClientDialog" width="40%" />

  * kaics.exe

      a _sub-pub_ message queue(_`MQ`_), which can penetrate the intranet, more info linked can get from [_here_](https://github.com/tsymiar/MyAutomatic/blob/auto-dev/LinxSrvc/IM/readme.md).

  * gSOAPverify

      a `SOAP-server` which is to verify login using the config file _myweb.wsdl_.

  * pthdtest.exe

      a thread pool based on `pthread`.

  * gn1

      a _cross-platform_, _big/small endian_, _increasing/decreasing_ binary number generator.

  * webevent_server

      a http server and client package manager, depends on `libevent`.

  * dpsk_chat

      a mini chat tool using _`DeepSeek`_ api to answer questions.

  * analyzing

      a code tool to compare differences between tow same-named files, or different directories.

      Not only thus tools, check `sometools`.

QtGames
-------

* [_`It`_](https://github.com/tsymiar/MyAutomatic/tree/auto-dev/QtGames) is a test-case using _`Qt`_, _`OpenGL`_. using _`mkallcase.sh`_ to build it.
  
## WinNTKline

##### [Microsoft .NET Framework 3.5](https://aka.ms/msbuild/developerpacks) is needed if compile WinNTKline

| CvMlwk |
|:----:|

> _`OpenCV`_ && some _`Machine Learning`_ learning cases.

| KlineUtil |
|:-------:|

> Utils of _cef-browser_, security libs, show _K-line_ by gl, simulate to _ctp_ ... _etc._

| WPFKline |
|:--------:|

> A K-line application using _`C#`_.

| TestUtils |
|:--------:|
> A testcases to test interface of _KlineUtil_ .

-------

#### _**I**mpact of the program has built in [`Market`]:_

<img src="WinNTKline/image/impact.png" title="impact" height="80%" width="80%" align="middle"/>
