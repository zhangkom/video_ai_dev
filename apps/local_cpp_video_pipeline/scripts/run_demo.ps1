param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [int]$Width = 640,
    [int]$Height = 360,
    [double]$Fps = 25.0,
    [int]$MaxFrames = 300
)

$ErrorActionPreference = "Stop"

$AppDir = Split-Path -Parent $PSScriptRoot
$Workspace = Resolve-Path (Join-Path $AppDir "..\..")
$ReportsDir = Join-Path $Workspace "reports\local_cpp_video_pipeline"
$BuildScript = Join-Path $PSScriptRoot "build.ps1"
$Exe = & $BuildScript | Select-Object -Last 1

New-Item -ItemType Directory -Force -Path $ReportsDir | Out-Null

$Stem = [System.IO.Path]::GetFileNameWithoutExtension($InputPath)
$Output = Join-Path $ReportsDir "$Stem.annotated.mp4"
$Report = Join-Path $ReportsDir "$Stem.report.md"
$Csv = Join-Path $ReportsDir "$Stem.latency.csv"

& $Exe `
    --input $InputPath `
    --output $Output `
    --report $Report `
    --csv $Csv `
    --width $Width `
    --height $Height `
    --fps $Fps `
    --max-frames $MaxFrames

Write-Output "Output: $Output"
Write-Output "Report: $Report"
Write-Output "CSV: $Csv"
