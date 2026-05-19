# Low-Latency Video AI Inference System Roadmap

## Positioning

This workspace is for building a practical, demonstrable video AI inference system. The local machine has Intel graphics and no NVIDIA GPU, so the project will start with CPU/Intel-friendly backends while keeping the architecture compatible with Triton/TensorRT/CUDA on a future GPU server.

## Phase 1: Local MVP

Goal: run an end-to-end pipeline on this machine.

- Read a local video or camera stream.
- Decode frames with FFmpeg/PyAV.
- Run a lightweight ONNX model with ONNX Runtime CPU.
- Draw detection or segmentation results.
- Export an annotated video and a latency report.

Success criteria:

- One command runs the full pipeline.
- Report includes decode, preprocess, inference, postprocess, and total latency.
- Output can be inspected visually.

Current status:

- `apps/local_cpp_video_pipeline/` builds and runs as the first C++ MVP.
- FFmpeg is used through raw RGB pipes for decode and encode.
- The current backend is `demo-saliency`, a deterministic C++ demo backend used only to validate the pipeline.
- A sample run processed 120 frames at 640x360 and produced an annotated MP4 plus CSV/Markdown latency report.

Next implementation step:

- Replace `demo-saliency` with the first real inference backend, preferably ONNX Runtime C++ CPU first, then OpenVINO for Intel acceleration.

## Phase 2: Intel Acceleration

Goal: use this machine's Intel hardware effectively.

- Add OpenVINO backend.
- Compare ONNX Runtime CPU vs OpenVINO CPU/GPU/NPU where available.
- Test FFmpeg QSV decode/encode where practical.
- Add benchmark tables in `reports/`.

## Phase 3: Edge Inference Comparison

Goal: compare edge-style deployment options.

- Add ncnn backend.
- Add MNN backend.
- Use the same test video and model family where possible.
- Compare latency, memory, model conversion friction, and engineering complexity.

## Phase 4: Low-Latency Preview

Goal: make the result feel like a low-latency product, not only an offline benchmark.

- Add a local preview service.
- Start with HTTP/MJPEG or WebSocket preview if WebRTC setup would slow the core work.
- Add WebRTC preview after the processing loop is stable.
- Measure glass-to-glass or pipeline latency as honestly as possible.

## Phase 5: NVIDIA-Compatible Server Path

Goal: preserve the original Triton/TensorRT/CUDA/NPPI story.

- Keep a `triton_model_repo/` layout.
- Add Triton client code that can target a remote Triton server.
- Document TensorRT/CUDA/NPPI optimization hooks.
- On a future NVIDIA machine, replace the ONNX Runtime/OpenVINO server path with Triton/TensorRT and compare results.

## First Implementation Target

Build:

```text
apps/local_onnx_video_pipeline/
```

with:

- Python.
- PyAV or FFmpeg subprocess for decode.
- ONNX Runtime CPU.
- OpenCV for drawing and video output.
- JSON/CSV benchmark output.
