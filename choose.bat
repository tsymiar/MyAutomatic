<# : choose.bat
::: Interactive folder picker – sets environment variables for dependencies

@echo off
chcp 65001 >nul
title Choose
setlocal enabledelayedexpansion

setx MYAUTOMATIC "%CD%"
set "LIBS=VCINSTALLDIR BOOST CEFDIR QTDIR OPENCV OPENSSL PTHD_LIB86 JAVA_HOME ZLIB libPNG SDL2"
set "t=%LIBS%"

:loop
for /f "tokens=1*" %%a in ("!t!") do (
    echo Choose "%%a" path ...
    for /f "delims=" %%I in ('powershell -noprofile "iex (${%~f0} | out-string)"') do (
        echo "%%a = %%~I"
        setx %%a "%%~I"
        if /i "%%a"=="PTHD_LIB86" (
            set PTHD=%%~I
            set PTHD=!PTHD:~0,-8!
            setx PTHD "!PTHD!"
            set "PATH=!PATH!;%%~I\dll\x86"
        )
        if exist "%%~I\bin\Debug" (
            set "PATH=!PATH!;%%~I\bin\Debug"
        ) else if exist "%%~I\bin" (
            set "PATH=!PATH!;%%~I\bin"
        )
        if exist "%%~I\lib\Debug" (
            set "PATH=!PATH!;%%~I\lib\Debug"
        ) else if exist "%%~I\lib" (
            set "PATH=!PATH!;%%~I\lib"
        )
    )
    set "t=%%b"
)
if defined t goto :loop
goto :EOF

:: end Batch / begin PowerShell #>

Add-Type -AssemblyName System.Windows.Forms
$f = New-Object Windows.Forms.FolderBrowserDialog
[void]$f.ShowDialog()
$f.SelectedPath
