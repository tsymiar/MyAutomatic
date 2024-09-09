@echo off
set BATDIR=%~dp0
echo locate: @ %BATDIR%
echo[]
echo cleaning lib
rd /s /q lib
echo cleaning built
rd /s /q build
echo cleaning LinxSrvc
rd /s /q LinxSrvc\.vs
rd /s /q LinxSrvc\bin
rd /s /q LinxSrvc\gen
rd /s /q LinxSrvc\out
rd /s /q LinxSrvc\build
echo cleaning out caches
rd /s /q WinNTKline\.vs
rd /s /q WinNTKline\cache
rd /s /q WinNTKline\out
rd /s /q WinNTKline\x64
rd /s /q WinNTKline\x86
rd /s /q WinNTKline\Debug
rd /s /q WinNTKline\Release
rd /s /q "WinNTKline\Generated Files"
echo cleaning MFC
rd /s /q WinNTKline\MFC
rd /s /q "WinNTKline\MFCKline\Generated Files"
set "RegExp=-*.con[^fig]"
for /R "WinNTKline\MFCKline\" %%f in (*.con) do (
    setlocal enabledelayedexpansion
    echo %%f | findstr /r /C:"%RegExp%" >nul 2>&1
    if ERRORLEVEL 1 (
        echo save %%f
    ) else (
        del /f /q %%f
    )
    endlocal
)
echo cleaning WPF
rd /s /q WinNTKline\WPF
echo cleaning WPFKline
rd /s /q WinNTKline\WPFKline\WPFKline\.vs
rd /s /q WinNTKline\WPFKline\WPFKline\obj
echo cleaning Scadup
rd /s /q WinNTKline\Scadup\x64
echo cleaning TestUtils
del /f /q WinNTKline\TestUtils\*.con
echo cleaning QtCases
:: https://download.qt.io/archive/qt/5.14/5.14.2/qt-opensource-windows-x86-5.14.2.exe
rd /s /q QtCases\GeneratedFiles
rd /s /q QtCases\build
del /f /q QtCases\*.so
del /f /q QtCases\*.stash
echo ----- finish clean -----
