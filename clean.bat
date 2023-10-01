@echo off
set BATDIR=%~dp0
echo locate: @ %BATDIR%
cd %BATDIR%
echo clean build
rd /s /q build
echo clean LinxSrvc
rd /s /q LinxSrvc\out
echo clean cache
rd /s /q WinNTKline\cache
echo clean MFC
rd /s /q WinNTKline\MFC
echo clean WPF
rd /s /q WinNTKline\WPF
echo clean dlls
rd /s /q WinNTKline\Debug
echo clean WPFKline
rd /s /q WinNTKline\WPFKline\WPFKline\obj
echo clean QtCases
rd /s /q QtCases\GeneratedFiles
echo finish
