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
copy ..\MFCKline\cfg .\cfg
copy ..\MFCKline\data .\data
copy ..\MFCKline\cef\*.dll .\
copy ..\MFCKline\CTP\*.dll .\
copy ..\MFCKline\font\lib\*.dll .\