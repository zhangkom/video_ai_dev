# Local C++ Video Pipeline

This is the first C++ MVP for the low-latency video AI inference workspace.

The current implementation intentionally avoids OpenCV, FFmpeg SDK headers, and ONNX Runtime C++ SDK so it can build on a fresh Windows machine with only:

- `g++`
- `cmake`
- `ninja`
- `ffmpeg`

FFmpeg is used through pipes:

```text
input video
  -> ffmpeg decode to RGB24 raw frames
  -> C++ frame loop
  -> demo inference backend
  -> C++ overlay
  -> ffmpeg encode to MP4
  -> latency CSV + Markdown report
```

## Current Backend

`demo-saliency` is a deterministic C++ demo backend. It is not a neural network. It finds a high-contrast tile and returns one box so the video pipeline, timing, drawing, and reporting can be validated before adding ONNX Runtime/OpenVINO/ncnn/MNN.

The backend boundary is explicit in `IInferenceBackend`, so later inference backends can replace it without rewriting the decode/encode loop.

## Build

From repository root:

```powershell
.\apps\local_cpp_video_pipeline\scripts\build.ps1
```

## Run

```powershell
.\apps\local_cpp_video_pipeline\scripts\run_demo.ps1 -Input C:\path\input.mp4
```

Outputs are written under `reports/local_cpp_video_pipeline/` by default.

