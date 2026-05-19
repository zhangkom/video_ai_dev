param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$AppDir = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $AppDir "build"

$WinLibsBin = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter "g++.exe" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "WinLibs" } |
    Select-Object -First 1 |
    ForEach-Object { Split-Path -Parent $_.FullName }

if ($WinLibsBin) {
    $env:Path = "$WinLibsBin;$env:Path"
}

foreach ($tool in @("g++", "ffmpeg")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Required tool not found in PATH: $tool"
    }
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Exe = Join-Path $BuildDir "local_cpp_video_pipeline.exe"
if ($Configuration -ieq "Debug") {
    $CxxFlags = @("-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-O0", "-g")
} else {
    $CxxFlags = @("-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-O2")
}

& g++ @CxxFlags `
    (Join-Path $AppDir "src\main.cpp") `
    -o $Exe

if (-not (Test-Path $Exe)) {
    throw "Build finished but executable was not found: $Exe"
}

Write-Output $Exe
