@echo off
set BATDIR=%~dp0
echo @%BATDIR%
echo clean LinxSrvc
rd /q /s %BATDIR%LinxSrvc\obj
echo clean cache
rd /q /s %BATDIR%WinNTKline\cache
echo clean WPFKline
rd /q /s %BATDIR%WinNTKline\WPFKline\WPFKline\obj
echo finish
