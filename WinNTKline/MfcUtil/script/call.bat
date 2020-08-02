@echo off
if exist data (
echo
)else (
md data
)
if exist cfg (
echo
)else (
md cfg
)
copy ..\MfcUtil\cfg\*.ini .\cfg
copy ..\MfcUtil\data .\data
copy ..\MfcUtil\CTP\*.dll .\
copy ..\MfcUtil\WPF\*.dll .\
copy ..\MfcUtil\font\lib\*.dll .\
copy %OPENCV%\bin\Debug\*.dll .\
copy %CEFDIR%\Debug\libcef.dll .\
copy %CEFDIR%\Debug\chrome_elf.dll .\
copy %libPNG%\lib\Debug\libpng16.dll .\
copy %QTDIR%\bin\*d.dll .\
copy %PTHD_LIB86%\..\..\dll\x86\pthreadVC*.dll .\
