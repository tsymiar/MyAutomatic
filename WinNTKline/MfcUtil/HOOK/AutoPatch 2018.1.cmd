@echo off
@color 1F

Title JetBrains.Resharper v2018.1: AutoPatch

echo ========================================================================
echo JetBrains.Resharper v2018.1: AutoPatch
echo ========================================================================

rem Administrative permissions required. Detecting permissions...
net session >nul 2>&1
if %errorLevel% == 0 (
    rem Success: Administrative permissions confirmed.
) else (
    echo ERROR: Administrative permissions required ^!
    echo        Patching not performed ^!
    echo.
    echo INFO:  Run AutoPatch.cmd as ADMIN ^!
    echo.
    pause
    exit
)

if not exist "%LOCALAPPDATA%\JetBrains" (
     echo ERROR: Directory "%LOCALAPPDATA%\JetBrains" not found ^!
     echo.
     echo INFO:  Install ReSharper first ^!
     echo.
     pause
     exit
)

"%~dp0\sfk190.exe" rep -bin /06075908582A/209F8601002A/      -dir "%LOCALAPPDATA%\JetBrains\Installations" -yes -file JetBrains.Platform.Shell.dll
"%~dp0\sfk190.exe" rep -bin /0228CB3200060A/209F8601002A0A/  -dir "%LOCALAPPDATA%\JetBrains\Installations" -yes -file JetBrains.Platform.Shell.dll
"%~dp0\sfk190.exe" rep -bin /020328A31D0006/162A0000000000/  -dir "%LOCALAPPDATA%\JetBrains\Installations" -yes -file JetBrains.Platform.Shell.dll
 
rem Remove FloatingTicket obtained from LocalServer    
reg DELETE HKCU\Software\JetBrains\Shared\vAny  /f  1>NUL 2>NUL

rem TrialReset for all .NET products
reg ADD HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\{9656c84c-e0b4-4454-996d-977eabdf9e86} /v "cfa3a5e5-a36d-58a3-ac12-a230ebdecaca"  /d "" /f 1>NUL 2>NUL
reg ADD HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\{9656c84c-e0b4-4454-996d-977eabdf9e86} /v "d5429f73-7a07-5d7e-b0b3-8029490fc7bc"  /d "" /f 1>NUL 2>NUL
reg ADD HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\{9656c84c-e0b4-4454-996d-977eabdf9e86} /v "ce48f388-6dec-5036-8405-42bfcc177f61"  /d "" /f 1>NUL 2>NUL
reg ADD HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\{9656c84c-e0b4-4454-996d-977eabdf9e86} /v "9c7ade5e-a3b6-5675-8e47-e15e5f1de122"  /d "" /f 1>NUL 2>NUL
reg ADD HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\{9656c84c-e0b4-4454-996d-977eabdf9e86} /v "8f7078fb-94ac-5ffc-847f-200624d9ad3b"  /d "" /f 1>NUL 2>NUL
 
echo.
echo Patching finished ^!
echo.
echo.
pause

:end