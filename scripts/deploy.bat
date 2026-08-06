@echo off
setlocal
REM Requires admin privileges (writes to C:\Program Files\...\Plugins).
REM Run from an elevated command prompt, or right-click the script -> Run as administrator.

set "MAX_INSTALL=C:\Program Files\Autodesk\3ds Max 2027"
set "SRC=%~dp0..\build\output\Release\SelectSimilar.dlu"
set "SRC_FALLBACK=%~dp0..\build\output\SelectSimilar.dlu"
set "DST=%MAX_INSTALL%\Plugins\"

set "FINAL_SRC="
if exist "%SRC%" (
    set "FINAL_SRC=%SRC%"
) else if exist "%SRC_FALLBACK%" (
    set "FINAL_SRC=%SRC_FALLBACK%"
) else (
    echo Plugin not built yet. Run scripts\build.bat first.
    echo Expected: %SRC%
    exit /b 1
)

net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: This script must be run as administrator.
    exit /b 1
)

echo Copying SelectSimilar.dlu to 3ds Max Plugins folder...
copy /Y "%FINAL_SRC%" "%DST%"
if errorlevel 1 exit /b 1

echo.
echo Deployed. Restart 3ds Max to load the plugin.
echo Default hotkey: Shift+G  (rebindable in Customize UI ^> Keyboard ^> "Select Similar")
endlocal
