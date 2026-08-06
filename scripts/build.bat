@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."
set "BUILD_DIR=%ROOT_DIR%\build"

if "%MAXSDK_PATH%"=="" set "MAXSDK_PATH=C:\Program Files\Autodesk\3ds Max 2027 SDK\maxsdk"
set "MAX_INSTALL=C:\Program Files\Autodesk\3ds Max 2027"

if not exist "%MAXSDK_PATH%\include\maxapi.h" (
    echo 3ds Max SDK not found at '%MAXSDK_PATH%'.
    echo Download from https://aps.autodesk.com/developer/overview/3ds-max
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -G "Visual Studio 17 2022" -A x64 -B "%BUILD_DIR%" -S "%ROOT_DIR%" -DMAXSDK_PATH="%MAXSDK_PATH%" -DMAX_INSTALL_PATH="%MAX_INSTALL%"
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

echo.
echo Build complete. Plugin at: %BUILD_DIR%\output\Release\SelectSimilar.dlu
endlocal
