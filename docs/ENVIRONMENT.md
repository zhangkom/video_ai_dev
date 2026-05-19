# Environment Report

Date: 2026-05-14  
Workspace: `C:\work\workspace_own\workspace_video_ai_dev`

## Summary

This machine has no NVIDIA GPU. The local development path should therefore start with CPU and Intel GPU friendly components:

- FFmpeg for input, decode, frame extraction, and baseline encode.
- Python + ONNX Runtime for the first AI inference baseline.
- OpenVINO as the preferred Intel acceleration path to add next.
- ncnn and MNN as edge inference comparison targets after the baseline runs.
- Triton/TensorRT/CUDA/NPPI kept as architecture-compatible targets for a future NVIDIA machine or remote server, not as the first local runtime.

## Hardware

- OS: Windows 10 Pro, x64.
- CPU: Intel Core Ultra 7 155H, 16 cores, 22 logical processors.
- Memory: about 32 GB RAM.
- GPU: Intel Arc Graphics.
- NVIDIA GPU/tooling: not available locally.

## Installed Tools

Available:

- Python 3.12.6.
- Git 2.54.0.
- FFmpeg 8.1.1 full build.
- FFprobe 8.1.1.
- Docker CLI 29.4.1.
- WSL2 with Ubuntu 26.04 LTS.

Missing or not currently available in PATH:

- CMake.
- MSVC `cl`.
- CUDA `nvcc`.
- `nvidia-smi`.

Docker note:

- Docker Desktop CLI exists, but the Docker daemon was not running during detection.
- `docker info` could not connect to `dockerDesktopLinuxEngine`.

WSL note:

- Ubuntu is running under WSL2.
- WSL reported a localhost proxy warning in NAT mode.
- Linux build tools still need a clean follow-up check/install before using WSL as the C++ build environment.

## FFmpeg Capabilities

FFmpeg reports these hardware acceleration methods:

- `cuda`
- `vaapi`
- `dxva2`
- `qsv`
- `d3d11va`
- `opencl`
- `vulkan`
- `d3d12va`
- `amf`

For this machine, the practical local targets are:

- Intel Quick Sync Video (`qsv`) for H.264/H.265/AV1 related experiments.
- Direct3D acceleration (`d3d11va`, `d3d12va`) for decode paths.
- OpenCL/Vulkan as possible frame processing or inference-adjacent paths.

The FFmpeg build also includes software encoders such as `libx264`, `libx265`, `libsvtav1`, and Intel QSV encoders such as `h264_qsv`, `hevc_qsv`, `av1_qsv`, and `vp9_qsv`.

## Python Packages

Detected in the current Python environment:

- `numpy`: available.
- `onnxruntime`: available.
- `av`: available.
- `fastapi`: available.
- `uvicorn`: available.

Not detected yet:

- `opencv-python` / `cv2`.
- `onnx`.
- `openvino`.
- `torch`.
- `tensorflow`.
- `MNN`.
- `aiortc`.

## Local Development Strategy

Because there is no NVIDIA GPU, the first runnable target should be:

```text
local video file or camera
  -> FFmpeg / PyAV decode
  -> CPU preprocessing with NumPy/OpenCV
  -> ONNX Runtime CPU inference
  -> postprocess and overlay
  -> local output video or HTTP preview
  -> benchmark report
```

Then evolve to:

```text
ONNX Runtime CPU baseline
  -> OpenVINO on Intel CPU/GPU/NPU where available
  -> ncnn CPU/Vulkan
  -> MNN CPU/OpenCL/Vulkan
  -> WebRTC low-latency preview
  -> Triton/TensorRT remote or future NVIDIA path
```

## Immediate Gaps

Before building the full C++/edge comparison path, install or enable:

- CMake.
- A C++ compiler, preferably Visual Studio Build Tools on Windows or `build-essential` inside WSL.
- OpenCV Python package for the first MVP.
- OpenVINO Python package for Intel acceleration.
- ONNX tooling for model conversion and inspection.
- Docker Desktop daemon if we want containerized services.

## Re-run

Use this script to repeat the environment check:

```powershell
.\scripts\env_check.ps1 -OutputPath .\reports\environment_check.md
```
