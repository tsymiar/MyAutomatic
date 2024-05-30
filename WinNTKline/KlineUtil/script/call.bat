@echo off
mklink /d cfg ..\KlineUtil\cfg
if exist cfg (
    echo mk 'cfg' link
) else (
    md cfg
    copy ..\KlineUtil\cfg\*.ini .\cfg
)
mklink /d data ..\KlineUtil\data
if exist data (
    echo mk 'data' link
) else (
    md data
    copy ..\KlineUtil\data .\data
)
windeployqt QtCases.exe
if %ERRORLEVEL% equ 0 (
    echo deploy 'QtCases' done
) else (
    copy %QTDIR%\bin\*d.dll .\
)
copy ..\KlineUtil\CTP\*.dll .\
copy ..\KlineUtil\WPF\*.dll .\
copy ..\KlineUtil\font\lib\*.dll .\
copy %OPENSSL%\bin\*.dll .\
copy %OPENCV%\bin\Debug\*.dll .\
copy %CEFDIR%\Debug\libcef.dll .\
copy %CEFDIR%\Debug\chrome_elf.dll .\
copy %libPNG%\lib\Debug\libpng16.dll .\
copy %PTHD_LIB86%\..\..\dll\x86\pthreadVC*.dll .\
