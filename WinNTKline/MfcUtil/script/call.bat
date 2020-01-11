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
copy ..\MfcUtil\cfg\*.dll .\
copy ..\MfcUtil\data .\data
copy ..\MfcUtil\cef\*.dll .\
copy ..\MfcUtil\CTP\*.dll .\
copy ..\MfcUtil\WPF\*.dll .\
copy ..\MfcUtil\font\lib\*.dll .\
copy %OPENCV%\bin\Debug\*.dll .\
