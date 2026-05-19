param(
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Continue"

function Write-Section {
    param([string]$Title)
    ""
    "## $Title"
}

function Test-Command {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) {
        return "$Name`t$($cmd.Source)"
    }
    return "$Name`tNOT_FOUND"
}

$lines = New-Object System.Collections.Generic.List[string]

$lines.Add("# Environment Check")
$lines.Add("")
$lines.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')")

$lines.Add((Write-Section "Windows"))
$computer = Get-ComputerInfo | Select-Object WindowsProductName, WindowsVersion, OsHardwareAbstractionLayer, CsSystemType, CsTotalPhysicalMemory
$lines.Add(($computer | Format-List | Out-String).Trim())

$lines.Add((Write-Section "CPU"))
$lines.Add(((Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed | Format-List | Out-String).Trim()))

$lines.Add((Write-Section "GPU"))
$lines.Add(((Get-CimInstance Win32_VideoController | Select-Object Name, AdapterRAM, DriverVersion, VideoProcessor | Format-List | Out-String).Trim()))

$lines.Add((Write-Section "Commands"))
foreach ($name in @("python", "py", "git", "cmake", "ffmpeg", "ffprobe", "docker", "wsl", "nvidia-smi", "nvcc", "cl")) {
    $lines.Add((Test-Command $name))
}

$lines.Add((Write-Section "Versions"))
foreach ($cmd in @(
    "python --version",
    "py --version",
    "git --version",
    "cmake --version",
    "ffmpeg -version",
    "ffprobe -version",
    "docker --version",
    "wsl --status"
)) {
    $lines.Add("")
    $lines.Add("`$ $cmd")
    try {
        $lines.Add(((Invoke-Expression $cmd 2>&1 | Select-Object -First 3 | Out-String).Trim()))
    } catch {
        $lines.Add($_.Exception.Message)
    }
}

$lines.Add((Write-Section "FFmpeg Hardware Acceleration"))
try {
    $lines.Add(((ffmpeg -hide_banner -hwaccels 2>&1 | Out-String).Trim()))
} catch {
    $lines.Add($_.Exception.Message)
}

$lines.Add((Write-Section "Python Packages"))
$py = @'
import importlib.util
mods = ["numpy", "cv2", "onnx", "onnxruntime", "openvino", "torch", "tensorflow", "MNN", "aiortc", "av", "fastapi", "uvicorn"]
for m in mods:
    print(f"{m}\t{bool(importlib.util.find_spec(m))}")
'@
try {
    $lines.Add((($py | python - 2>&1 | Out-String).Trim()))
} catch {
    $lines.Add($_.Exception.Message)
}

if ($OutputPath) {
    $dir = Split-Path -Parent $OutputPath
    if ($dir) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    $lines -join "`r`n" | Set-Content -LiteralPath $OutputPath -Encoding UTF8
    Write-Output $OutputPath
} else {
    $lines -join "`r`n"
}
