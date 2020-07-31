@echo off
set BATDIR=%~dp0
echo locate: @ %BATDIR%
cd %BATDIR%
echo clean LinxSrvc
rd /s /q LinxSrvc\obj
echo clean cache
rd /s /q WinNTKline\cache
echo clean WPFKline
rd /s /q WinNTKline\WPFKline\WPFKline\obj
echo clean QtCases
rd /s /q QtCases\GeneratedFiles
echo finish
