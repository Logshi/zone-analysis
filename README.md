# Zone Analysis — RTSP Dwell-Time Person Detection with C++, YOLO ONNX and OpenCV

A C++ application that analyzes RTSP streams or video files, detects people inside user-defined polygon zones, and flags when a person stays longer than a configured dwell-time threshold. It saves alert snapshots and writes a CSV event log.

## Overview

Many surveillance and analytics scenarios don't just need *"is there a person?"* — they need *"is a person lingering in this specific area for too long?"*. This project addresses that by combining object detection with zone geometry and time tracking, producing actionable events (snapshots + logs) instead of raw detections.

## Motivation

I built this to develop a complete, real-time computer vision pipeline in C++ rather than a notebook prototype. It exercises detection, geometric reasoning, temporal tracking, file I/O and stream handling together — the kind of end-to-end engineering required for edge deployment.

## Features

- RTSP stream and local video file input
- YOLO inference via ONNX for person detection
- User-defined polygon zones
- Dwell-time tracking per person/zone
- Alert snapshot capture when the dwell threshold is exceeded
- CSV event logging for downstream analysis
- Real-time OpenCV processing pipeline

## Architecture

```
RTSP / Video Input
        |
   OpenCV Capture
        |
  YOLO ONNX Inference  --->  Person Detections
        |
  Zone Membership Check (polygon)
        |
  Dwell-Time Tracker (per person/zone)
        |
   +----+----+
   |         |
Snapshot   CSV Event Log
 (alerts)   (timestamped)
```

## Tech Stack

- **Language:** C++
- **Computer Vision:** OpenCV
- **AI/ML:** YOLO (ONNX runtime inference)
- **Input:** RTSP / video files
- **Output:** image snapshots, CSV logs
- **Build:** CMake

## Installation

> Adjust to match the actual build setup in this repository.

```bash
git clone https://github.com/Logshi/zone-analysis.git
cd zone-analysis
mkdir build && cd build
cmake ..
cmake --build .
```

Dependencies (typical): OpenCV with the appropriate ONNX inference backend. Place your YOLO `.onnx` model where the application expects it (see config).

## Usage

```bash
# Example — adapt to the actual CLI arguments
./zone-analysis --input rtsp://<camera-url> --model model.onnx --zones zones.json
```

Configure zones, the dwell-time threshold, and output paths via the project's configuration before running.

## Demo

> Placeholders — replace with real captures (no real faces or private locations).

- `docs/demo.gif`
- `docs/screenshots/zone-detection.png`
- `docs/screenshots/alert-snapshot.png`

## Results

Benchmark planned. Real measurements (FPS, latency, detection behavior across stream types) will be documented here once collected. No fabricated numbers are included.

## Roadmap

- [ ] Multi-zone and multi-stream support
- [ ] Configurable model backends
- [ ] RTSP reconnection handling
- [ ] Unit tests and CI
- [ ] Documented benchmarks on target hardware

## What I Learned

- Building a real-time video pipeline in C++ with OpenCV
- Integrating ONNX-based YOLO inference into a C++ application
- Point-in-polygon geometry for zone membership
- Temporal state tracking (dwell-time) across frames
- Producing useful artifacts (snapshots, structured logs) from a CV pipeline

## Security & Privacy

This repository contains no API keys, customer data, real camera footage, identifiable faces, or private deployment details. Sample inputs (if any) are synthetic or non-sensitive.

## License

MIT — see [LICENSE](LICENSE).
