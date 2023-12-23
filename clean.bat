@echo off
set BATDIR=%~dp0
echo locate: @%BATDIR%
cd %BATDIR%
echo[
echo cleaning LinxSrvc
rd /s /q LinxSrvc\build
rd /s /q LinxSrvc\gn\out
echo cleaning build caches
rd /s /q WinNTKline\.vs
rd /s /q WinNTKline\cache
rd /s /q WinNTKline\out
rd /s /q WinNTKline\x64
rd /s /q WinNTKline\x86
rd /s /q WinNTKline\Debug
echo cleaning MFC
rd /s /q WinNTKline\MFC
echo cleaning WPF
rd /s /q WinNTKline\WPF
echo cleaning WPFKline
rd /s /q WinNTKline\WPFKline\WPFKline\obj
echo cleaning QtCases
rd /s /q QtCases\GeneratedFiles
echo script finish clean
