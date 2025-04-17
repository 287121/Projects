@echo off
:: Check for administrative privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting admin privileges...
    powershell -Command "Start-Process '%~f0' -Verb runAs"
    exit /b
)

:: Get the current directory
set "folder=%~dp0"
if "%folder:~-1%"=="\" set "folder=%folder:~0,-1%"

echo Removing Windows Defender exclusion for: %folder%
powershell -Command "Remove-MpPreference -ExclusionPath '%folder%'"

echo Done.
pause
