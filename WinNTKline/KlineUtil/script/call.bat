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
copy ..\KlineUtil\cfg\*.ini .\cfg
copy ..\KlineUtil\data .\data
copy ..\KlineUtil\CTP\*.dll .\
copy ..\KlineUtil\WPF\*.dll .\
copy ..\KlineUtil\font\lib\*.dll .\
copy %OPENSSL%\bin\*.dll .\
copy %OPENCV%\bin\Debug\*.dll .\
copy %CEFDIR%\Debug\libcef.dll .\
copy %CEFDIR%\Debug\chrome_elf.dll .\
copy %libPNG%\lib\Debug\libpng16.dll .\
copy %QTDIR%\bin\*d.dll .\
copy %PTHD_LIB86%\..\..\dll\x86\pthreadVC*.dll .\
