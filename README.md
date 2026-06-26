# Zone Analysis

Detects people in an RTSP camera, webcam, or video file using YOLO ONNX +
OpenCV DNN. If a detected person stays inside a user-defined polygon region
longer than a configured duration, the app raises an alert, saves a
snapshot, and writes a CSV log entry.

## Features

- RTSP camera, local video file, or webcam input
- Runs an ONNX model via OpenCV DNN
- Tracks only the COCO `person` class
- Dwell-time alert when someone stays inside the polygon region
- Alert snapshots: `alerts/*.jpg`
- Alert log: `alerts/alerts.csv`
- Interactive mouse-based polygon drawing, or a fixed `--region` from the CLI
- Setup/build scripts for Windows, Ubuntu/Debian, and Raspberry Pi

## Hardware Requirements

By default the app runs OpenCV DNN with `DNN_BACKEND_OPENCV` and
`DNN_TARGET_CPU`, so a GPU is not required. Every frame is resized to
640x640 before being run through the model.

Research notes:

- OpenCV DNN supports loading ONNX models and targeting the CPU:
  https://docs.opencv.org/4.x/d6/d0f/group__dnn.html
- Ultralytics YOLO26n benchmark figures at 640px: 2.4M parameters,
  5.4 GFLOPs, CPU ONNX 38.9 ms, T4 TensorRT 1.7 ms:
  https://docs.ultralytics.com/models/yolo26/
- OpenCV's Linux install guide requires CMake and a C++ compiler:
  https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html
- OpenCV's Windows install guide covers CMake, Git, and a Visual
  Studio/C++ compiler workflow:
  https://docs.opencv.org/4.x/d3/d52/tutorial_windows_install.html
- The Ubuntu release cycle page lists the support windows for 26.04 LTS
  and 24.04 LTS:
  https://ubuntu.com/about/release-cycle
- NVIDIA's CUDA Linux docs list Ubuntu 26.04 LTS, 24.04 LTS, and 22.04 LTS
  among the supported Linux distributions for CUDA:
  https://docs.nvidia.com/cuda/cuda-installation-guide-linux/
- OpenVINO's system requirements list Ubuntu 24.04/22.04 LTS support and
  extra driver requirements for Intel GPU/NPU:
  https://docs.openvino.ai/2025/about-openvino/release-notes-openvino/system-requirements.html
- Debian's release page lists Debian 13 `trixie` as the current stable
  release and states the Debian release life cycle is 5 years:
  https://www.debian.org/releases/
- The Linux Mint download page lists Linux Mint 22.3 `Zena` as the
  current recommended release:
  https://linuxmint.com/download.php
- The Linux Mint 22.3 release notes state the release is supported as an
  LTS until April 2029 and that Linux Mint 22.x is based on Ubuntu 24.04:
  https://linuxmint.com/rel_zena.php
- The LMDE 7 page states Linux Mint Debian Edition uses a Debian package
  base instead of Ubuntu:
  https://linuxmint.com/download_lmde.php
- Raspberry Pi's docs describe the Raspberry Pi OS install and apt-based
  package management flow:
  https://www.raspberrypi.com/documentation/computers/getting-started.html
- The Raspberry Pi 5 product page lists 4 GB, 8 GB, and 16 GB RAM options:
  https://www.raspberrypi.com/products/raspberry-pi-5/

Practical minimum hardware:

| Component | Minimum |
| --- | --- |
| CPU | 64-bit, 4-core Intel i5 / AMD Ryzen 3 or equivalent |
| RAM | 8 GB |
| GPU | Not required; the current code runs on CPU |
| Disk | 1 GB free for runtime; 25-40 GB recommended on Windows for vcpkg/OpenCV build |
| Camera/network | 1 RTSP camera or USB webcam; a stable LAN/Wi-Fi for RTSP |
| OS | Windows 10/11 x64 or Ubuntu Server/Desktop 24.04/26.04 LTS |

Recommended hardware:

| Use case | Recommendation |
| --- | --- |
| Single camera, 640x640, low/medium FPS | Modern 6-core CPU, 16 GB RAM |
| Single camera, smoother tracking | Intel i5 10th gen+ / Ryzen 5 3600+ or equivalent |
| Multiple cameras or high FPS | 8+ core CPU, 16-32 GB RAM; GPU acceleration would require adapting the code for CUDA/TensorRT/OpenVINO |

Expected performance depends on the hardware, camera resolution, RTSP codec,
and number of people in the scene. The YOLO26n CPU ONNX benchmark only
covers model inference time; video decode, resize, tracking, drawing, and
display can lower the overall FPS.

## Best Linux Choices

For a field deployment, the cleanest option is a GUI-less Ubuntu Server LTS.
This app opens an OpenCV window, so install a desktop environment if you
need to watch the feed on the device itself; for edge scenarios that only
need alerts/logs, headless operation uses fewer resources. To run headless,
use `--no-gui` (the app skips `cv::imshow`/`cv::waitKey` entirely in that
mode).

| Scenario | Best Linux choice | Why |
| --- | --- | --- |
| New field deployment | Ubuntu Server 26.04 LTS x86_64 | Latest LTS, 5 years of standard security maintenance, current kernel and hardware support |
| Lowest-risk package/driver compatibility | Ubuntu Server 24.04 LTS x86_64 | Long-established LTS, broad support in OpenVINO/CUDA docs, more mature package ecosystem |
| Low-resource mini PC | Ubuntu Server 24.04/26.04 LTS minimal install | Reduces desktop overhead, leaving CPU/RAM for RTSP + CPU inference |
| Development/testing | Ubuntu Desktop 24.04/26.04 LTS or WSL2 | Easier GUI and debugging; WSL2 should not be the first choice for field/RTSP deployment |
| Considering Intel GPU/NPU acceleration | Ubuntu 24.04 LTS or 22.04 LTS | OpenVINO docs support these LTS releases for Intel GPU/NPU; the current code would need to be adapted for OpenVINO |
| Considering NVIDIA GPU/TensorRT | Ubuntu 24.04 LTS or 26.04 LTS | CUDA 13.3 docs support these Ubuntu LTS releases; the current code would need to be adapted for CUDA/TensorRT |

Debian, Linux Mint, and similar distributions:

| Distribution | Suitability | Note |
| --- | --- | --- |
| Debian 13 `trixie` stable | Well suited for field/edge CPU-only setups | Current stable Debian; a server/minimal install uses few resources. `scripts/setup_ubuntu.sh` generally also works on Debian since it just installs apt packages. |
| Debian 12 `bookworm` oldstable | Fine for older, already-validated systems | Usable if hardware/driver compatibility has already been tested; prefer Debian 13 for new installs. |
| Linux Mint 22.3 `Zena` Cinnamon | Good for desktop users | Ubuntu 24.04-based LTS; comfortable for GUI camera monitoring and demos, but Cinnamon uses more resources on a field device. |
| Linux Mint 22.3 Xfce | Better for older/low-resource desktops | Mint's lightest desktop option; a more sensible choice than Cinnamon on mini PCs that need on-screen monitoring. |
| Linux Mint 22.3 MATE | Balanced desktop option | A middle ground that's a bit heavier than Xfce but lighter than Cinnamon. |
| LMDE 7 | Good for those who want Mint on a Debian base | Uses a Debian package base instead of Ubuntu; worth considering if you want desktop convenience plus a Debian base. |
| MX Linux / antiX / similar Debian-based distros | Experienced users only | Should work if apt/OpenCV/FFmpeg packages are available; this repo's deployment docs aren't as directly tested here as on Debian/Ubuntu/Mint. |
| Arch / Manjaro / Fedora / openSUSE | Workable but not the first choice | Package names and the OpenCV build can differ; this repo's automated scripts are written for apt-based systems. |

Install note for Debian/Mint:

```bash
sudo bash scripts/setup_ubuntu.sh
bash scripts/build_ubuntu.sh
bash scripts/run_ubuntu.sh "rtsp://user:password@192.168.1.50:554/stream1"
```

Even though the script is named `setup_ubuntu.sh`, it installs the same apt
packages on Debian, Linux Mint, LMDE, and Ubuntu derivatives:
`build-essential`, `cmake`, `pkg-config`, `libopencv-dev`, `ffmpeg`.

Practical device recommendations for Linux:

| Profile | Recommended device class |
| --- | --- |
| 1 camera / CPU-only | Intel N100/N150 mini PC, Intel i3 10th gen+, Ryzen 3 4300U+; 8-16 GB RAM; NVMe SSD |
| 1-2 cameras / smoother tracking | Intel i5 10th gen+, Ryzen 5 3600/5600U+; 16 GB RAM; wired Ethernet |
| 3+ cameras / high FPS | Intel i7/Ryzen 7 or a system with an edge GPU; 32 GB RAM; dedicated network bandwidth per camera |
| Field/edge box | Fanned or well-cooled mini PC, UPS, wired Ethernet, a systemd service for auto-start |
| Raspberry Pi | Raspberry Pi 5 8 GB or 16 GB; an experimental/budget option for a single RTSP camera in headless mode |

Linux notes:

- For field deployment, Ubuntu Server LTS carries less background overhead
  than Ubuntu Desktop. If you don't need to watch the app on screen, prefer
  a server/minimal install.
- Ubuntu LTS releases ship every two years and get 5 years of standard
  security maintenance, so prefer an LTS over an interim release for field
  deployment.
- For RTSP cameras, prefer wired Ethernet, a static IP or DHCP reservation,
  and a low-latency connection on the same LAN.
- NVIDIA/Intel GPU acceleration is not active in the current code. If you're
  buying hardware, test CPU-only performance first, then plan a
  CUDA/TensorRT or OpenVINO integration if you actually need it.

## Raspberry Pi Setup

Recommended starting point for Raspberry Pi:

| Component | Recommendation |
| --- | --- |
| Board | Raspberry Pi 5, 8 GB or 16 GB RAM |
| OS | Raspberry Pi OS Lite 64-bit, latest stable |
| Storage | 32 GB+ microSD; a USB/NVMe SSD for a more stable field deployment |
| Network | Wired Ethernet recommended for the RTSP camera |
| Cooling | Active cooling recommended for the Raspberry Pi 5 |
| Mode | Headless `--no-gui` by default |

It can also build and run on a Raspberry Pi 4 4 GB, but YOLO ONNX CPU
inference performance is limited. For a smoother real-time experience, pick
a Raspberry Pi 5 8 GB+. For multiple cameras or high FPS, a mini PC is the
better choice.

1. Install Raspberry Pi OS.

   Use Raspberry Pi Imager and pick `Raspberry Pi OS Lite (64-bit)`. Enable
   SSH and set a username/password or SSH key. After first boot:

   ```bash
   sudo apt update
   sudo apt full-upgrade -y
   sudo reboot
   ```

2. Clone the repo.

   ```bash
   git clone https://github.com/Logshi/zone-analysis.git
   cd zone-analysis
   ```

3. Install required packages.

   ```bash
   sudo bash scripts/setup_rpi.sh
   ```

   This installs: `build-essential`, `cmake`, `git`, `pkg-config`,
   `libopencv-dev`, `ffmpeg`.

4. Build OpenCV from source (required for the YOLOv8 model to work).

   The apt-installed `libopencv-dev` on Raspberry Pi OS bookworm is OpenCV
   4.6.0, whose `cv::dnn` cannot run YOLOv8's detection head (it throws a
   `shape_utils.hpp` assertion on every frame instead of crashing once, since
   the app catches and skips the bad frame, hiding the fact that *every*
   frame fails). This has been confirmed both on Raspberry Pi OS bookworm
   (OpenCV 4.6.0) and on Ubuntu 22.04 (OpenCV 4.5.4); OpenCV 4.10+ does not
   have this problem. Build and install a newer OpenCV from source:

   ```bash
   bash scripts/build_opencv_rpi.sh
   ```

   This takes roughly 1-3+ hours on Raspberry Pi hardware and needs a few GB
   of free disk space. It installs OpenCV 4.10.0 to `/usr/local`, which
   CMake picks up ahead of the older apt copy.

5. Build the app.

   ```bash
   rm -rf build/rpi
   bash scripts/build_rpi.sh
   ```

   The CMake configure output should report `Found OpenCV: ... (found
   version "4.10.0")` — if it still says 4.6.0, the source-built copy in
   `/usr/local` isn't being found ahead of the apt one; check that step 4
   completed (`sudo make install` and `sudo ldconfig`) without errors.

6. Run headless with an RTSP camera.

   ```bash
   bash scripts/run_rpi.sh "rtsp://user:password@192.168.1.50:554/stream1"
   ```

   In headless mode no window opens; if an alert fires, a snapshot is
   written to `alerts/` and a log entry is appended to `alerts/alerts.csv`.
   Press `Ctrl+C` to quit.

7. Try it with a USB webcam.

   ```bash
   bash scripts/run_rpi.sh "0"
   ```

8. Run with a GUI on a Raspberry Pi with a desktop installed.

   ```bash
   bash scripts/run_rpi.sh --gui "rtsp://user:password@192.168.1.50:554/stream1"
   ```

   Without `--region`, this drops into interactive polygon-drawing mode on
   the live feed: left click adds a point, right click undoes the last one,
   `c` confirms, `r` resets, ESC falls back to the default region.

Performance and stability notes:

- Prefer a 640x360 or 720p low-bitrate stream from the RTSP camera; the app
  resizes the frame to 640x640 anyway.
- Put the camera and the Raspberry Pi on the same LAN, and use wired
  Ethernet instead of Wi-Fi if possible.
- Since the Raspberry Pi runs CPU-only, FPS can be low; test with a single
  camera first.
- Use active cooling and a quality power supply for long-running setups.
- The Raspberry Pi Camera Module isn't RTSP directly; for this app, an RTSP
  camera or USB webcam is the easiest source.
- If the RTSP source has packet loss (visible as `decode_slice_header
  error` / `Missing reference picture` in the logs), push the stream over
  TCP and set `OPENCV_FFMPEG_CAPTURE_OPTIONS="rtsp_transport;tcp"` before
  running the app; a corrupted frame is skipped rather than crashing the
  app either way.

## Repo Layout

```text
.
|-- CMakeLists.txt
|-- README.md
|-- vcpkg.json
|-- requirements-python.txt
|-- models/
|   |-- yolov8n.onnx
|   |-- yolo26n.onnx
|   `-- README.md
|-- alerts/
|   `-- .gitkeep
|-- scripts/
|   |-- setup_windows.ps1
|   |-- build_windows.ps1
|   |-- run_windows.ps1
|   |-- setup_ubuntu.sh
|   |-- build_ubuntu.sh
|   |-- run_ubuntu.sh
|   |-- setup_rpi.sh
|   |-- build_rpi.sh
|   `-- run_rpi.sh
`-- src/
    |-- main.cpp
    |-- camera.cpp / camera.hpp
    |-- detector.cpp / detector.hpp
    |-- tracker.cpp / tracker.hpp
    `-- notifier.cpp / notifier.hpp
```

## Quick Start: Windows

Open PowerShell in the repo folder:

```powershell
cd C:\Users\<user>\zone-analysis
Set-ExecutionPolicy -Scope Process Bypass -Force
.\scripts\setup_windows.ps1 -InstallSystemTools
.\scripts\build_windows.ps1
```

Run with an RTSP camera:

```powershell
.\scripts\run_windows.ps1 -Source "rtsp://user:password@192.168.1.50:554/stream1"
```

Run with a video file:

```powershell
.\scripts\run_windows.ps1 -Source "C:\video\test.mp4"
```

Run with a webcam:

```powershell
.\scripts\run_windows.ps1 -Source "0"
```

`setup_windows.ps1 -InstallSystemTools` attempts to install:

- Git
- CMake
- Visual Studio 2022 Build Tools C++ workload
- A local `.deps/vcpkg` folder
- OpenCV 4 via vcpkg

Note: the Visual Studio Build Tools and OpenCV install can take a while.

## Quick Start: Ubuntu / WSL

```bash
cd ~/zone-analysis
sudo bash scripts/setup_ubuntu.sh
bash scripts/build_ubuntu.sh
```

With an RTSP camera:

```bash
bash scripts/run_ubuntu.sh "rtsp://user:password@192.168.1.50:554/stream1"
```

With a video file:

```bash
bash scripts/run_ubuntu.sh "/home/user/test.mp4"
```

With a webcam:

```bash
bash scripts/run_ubuntu.sh "0"
```

## Run Options

Main executable usage:

```bash
dwell_alert <source> <onnx_model_path> [--dwell seconds] [--region "x1,y1;x2,y2;x3,y3"] [--alerts alerts_dir] [--no-gui]
```

Example:

```bash
./build/unix/dwell_alert "rtsp://user:pass@192.168.1.50:554/stream1" models/yolov8n.onnx --dwell 15 --region "180,200;500,200;560,560;120,560" --alerts alerts
```

Options:

- `source`: RTSP URL, video file path, or webcam index (`0`, `1`, ...)
- `onnx_model_path`: recommended default is `models/yolov8n.onnx`
- `--dwell`: how long someone must stay in the region to trigger an alert, default `10`
- `--region`: polygon points in the 640x640 frame's coordinate space
- `--alerts`: snapshot/CSV log directory, default `alerts`
- `--conf`: detection confidence threshold (0-1), default `0.5`; raise it
  (e.g. `0.6`-`0.7`) if non-people are being detected as a person
- `--no-gui`: runs headless without opening an OpenCV window; recommended
  for Raspberry Pi OS Lite/SSH

If `--region` is omitted and the GUI is enabled, the app opens an
interactive polygon-drawing mode on the live feed before tracking starts:
left click adds a point, right click undoes the last one, `c` confirms,
`r` resets, ESC falls back to the default built-in region.

Press `q` or `ESC` to quit in GUI mode. In `--no-gui` mode, use `Ctrl+C`
from the terminal.

## Python ONNX Test

The main app runs in C++. If you want to quickly test the ONNX model on a
single image, you can use the Python helper script:

```bash
python -m venv .venv
.\.venv\Scripts\activate
pip install -r requirements-python.txt
python pyt.py --image C:\path\to\image.jpg --model models/yolov8n.onnx --save runs
```

On Linux/macOS:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements-python.txt
python pyt.py --image /path/to/image.jpg --model models/yolov8n.onnx --save runs
```

## Model

The repo includes both `models/yolov8n.onnx` (recommended default) and
`models/yolo26n.onnx`. See `models/README.md` for why `yolo26n.onnx` can
fail to load on older OpenCV DNN builds. To use a different YOLOv8/YOLO11
ONNX model, export it with a 640 input size and pass that file via
`--model`/the second CLI argument instead.

Example ONNX export with Ultralytics:

```bash
pip install ultralytics
yolo export model=yolov8n.pt format=onnx imgsz=640
```

## GitHub Repo Description

`Zone-based RTSP/video dwell-time person detection with YOLO ONNX and OpenCV; saves alert snapshots and CSV logs.`
