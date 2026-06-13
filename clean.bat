@echo off
setlocal enabledelayedexpansion
set "ROOT=%~dp0"
echo Cleaning workspace at %ROOT%

call :safe_rd lib
call :safe_rd build

echo --- LinxSrvc ---
call :safe_rd LinxSrvc\.vs
call :safe_rd LinxSrvc\bin
call :safe_rd LinxSrvc\gen
call :safe_rd LinxSrvc\out
call :safe_rd LinxSrvc\build

echo --- WinNTKline ---
call :safe_rd WinNTKline\.vs
call :safe_rd WinNTKline\cache
call :safe_rd WinNTKline\out
call :safe_rd WinNTKline\x64
call :safe_rd WinNTKline\x86
call :safe_rd WinNTKline\Debug
call :safe_rd WinNTKline\Release
call :safe_rd "WinNTKline\Generated Files"
call :safe_rd WinNTKline\MFC
call :safe_rd "WinNTKline\MFCKline\Generated Files"

echo --- MFCKline .con cleanup ---
for /R "WinNTKline\MFCKline\" %%f in (*.con) do (
    echo %%f | findstr /r /C:"-.*\.con[^f]" >nul 2>&1
    if errorlevel 1 (
        echo   keep %%f
    ) else (
        del /f /q "%%f"
    )
)

echo --- WPF / WPFKline ---
call :safe_rd WinNTKline\WPF
call :safe_rd WinNTKline\WPFKline\WPFKline\.vs
call :safe_rd WinNTKline\WPFKline\WPFKline\obj

echo --- Scadup / TestUtils ---
call :safe_rd WinNTKline\Scadup\x64
if exist WinNTKline\TestUtils\*.con del /f /q WinNTKline\TestUtils\*.con

echo --- QtGames ---
call :safe_rd QtGames\GeneratedFiles
call :safe_rd QtGames\build
if exist QtGames\*.so     del /f /q QtGames\*.so
if exist QtGames\*.stash  del /f /q QtGames\*.stash

echo ----- Clean finished -----
goto :EOF

:safe_rd
if exist "%~1" rd /s /q "%~1" 2>nul
goto :EOF
