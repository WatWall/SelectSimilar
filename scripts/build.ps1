$ErrorActionPreference = "Stop"

$MAXSDK = if ($env:MAXSDK_PATH) { $env:MAXSDK_PATH } else { "C:\Program Files\Autodesk\3ds Max 2027 SDK\maxsdk" }
$MAX_INSTALL = "C:\Program Files\Autodesk\3ds Max 2027"

if (-not (Test-Path "$MAXSDK\include\maxapi.h")) {
    Write-Error "3ds Max SDK not found at '$MAXSDK'. Download from https://aps.autodesk.com/developer/overview/3ds-max"
    exit 1
}

$buildDir = Join-Path $PSScriptRoot "..\build"
$resolved = (Resolve-Path $PSScriptRoot).Path
$buildDir = Join-Path $resolved "..\build"
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

cmake -G "Visual Studio 17 2022" -A x64 `
    -B $buildDir `
    -S $resolved `
    "-DMAXSDK_PATH=$MAXSDK" `
    "-DMAX_INSTALL_PATH=$MAX_INSTALL"

cmake --build $buildDir --config Release

Write-Output "`nBuild complete. Plugin at: $buildDir\output\Release\"
Write-Output "Look for SelectSimilar<MaxYear>.dlu (e.g. SelectSimilar2027.dlu)"
