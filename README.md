# Zone Analysis

RTSP kamera, webcam veya video dosyasi uzerinde YOLO ONNX + OpenCV DNN ile insan
tespiti yapar. Tespit edilen kisi belirlenen polygon bolgede belirlenen sureden
fazla kalirsa alarm uretir, snapshot kaydeder ve CSV log yazar.

## Ozellikler

- RTSP kamera, yerel video dosyasi veya webcam girisi
- OpenCV DNN ile ONNX model calistirma
- Sadece COCO `person` sinifi icin takip
- Polygon bolge icinde dwell-time alarmi
- Alarm snapshotlari: `alerts/*.jpg`
- Alarm logu: `alerts/alerts.csv`
- Windows ve Ubuntu icin kurulum/derleme scriptleri

## Repo Yapisi

```text
.
|-- CMakeLists.txt
|-- README.md
|-- vcpkg.json
|-- requirements-python.txt
|-- models/
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
|   `-- run_ubuntu.sh
`-- src/
    |-- main.cpp
    |-- camera.cpp / camera.hpp
    |-- detector.cpp / detector.hpp
    |-- tracker.cpp / tracker.hpp
    `-- notifier.cpp / notifier.hpp
```

## Hizli Baslangic: Windows

PowerShell'i repo klasorunde acin:

```powershell
cd C:\Users\<kullanici>\zone-analysis
Set-ExecutionPolicy -Scope Process Bypass -Force
.\scripts\setup_windows.ps1 -InstallSystemTools
.\scripts\build_windows.ps1
```

RTSP kamera ile calistirma:

```powershell
.\scripts\run_windows.ps1 -Source "rtsp://user:password@192.168.1.50:554/stream1"
```

Video dosyasi ile calistirma:

```powershell
.\scripts\run_windows.ps1 -Source "C:\video\test.mp4"
```

Webcam ile calistirma:

```powershell
.\scripts\run_windows.ps1 -Source "0"
```

`setup_windows.ps1 -InstallSystemTools` sunlari kurmaya calisir:

- Git
- CMake
- Visual Studio 2022 Build Tools C++ araclari
- Yerel `.deps/vcpkg` klasoru
- vcpkg uzerinden OpenCV 4

Not: Visual Studio Build Tools ve OpenCV kurulumu uzun surebilir.

## Hizli Baslangic: Ubuntu / WSL

```bash
cd ~/zone-analysis
sudo bash scripts/setup_ubuntu.sh
bash scripts/build_ubuntu.sh
```

RTSP kamera ile:

```bash
bash scripts/run_ubuntu.sh "rtsp://user:password@192.168.1.50:554/stream1"
```

Video dosyasi ile:

```bash
bash scripts/run_ubuntu.sh "/home/user/test.mp4"
```

Webcam ile:

```bash
bash scripts/run_ubuntu.sh "0"
```

## Calistirma Opsiyonlari

Ana executable formati:

```bash
dwell_alert <source> <onnx_model_path> [--dwell seconds] [--region "x1,y1;x2,y2;x3,y3"] [--alerts alerts_dir]
```

Ornek:

```bash
./build/unix/dwell_alert "rtsp://user:pass@192.168.1.50:554/stream1" models/yolo26n.onnx --dwell 15 --region "180,200;500,200;560,560;120,560" --alerts alerts
```

Opsiyonlar:

- `source`: RTSP URL, video dosyasi yolu veya webcam indexi (`0`, `1`, ...)
- `onnx_model_path`: Varsayilan model `models/yolo26n.onnx`
- `--dwell`: Alarm icin bolgede kalma suresi, varsayilan `10`
- `--region`: 640x640 frame uzerinde polygon noktalaridir
- `--alerts`: Snapshot ve CSV log klasoru, varsayilan `alerts`

Programdan cikmak icin `q` veya `ESC` tusuna basin.

## Python ONNX Testi

Ana uygulama C++ ile calisir. Isterseniz ONNX modelini tek bir gorselde hizli
test etmek icin Python yardimci scriptini kullanabilirsiniz:

```bash
python -m venv .venv
.\.venv\Scripts\activate
pip install -r requirements-python.txt
python pyt.py --image C:\path\to\image.jpg --model models/yolo26n.onnx --save runs
```

Linux/macOS icin:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements-python.txt
python pyt.py --image /path/to/image.jpg --model models/yolo26n.onnx --save runs
```

## Model

Repo icinde `models/yolo26n.onnx` vardir. Farkli YOLOv8/YOLO11 ONNX modeli
kullanmak isterseniz modeli 640 input boyutuyla export edip `--model` yerine
o dosyayi verebilirsiniz.

Ultralytics ile ONNX export ornegi:

```bash
pip install ultralytics
yolo export model=yolov8n.pt format=onnx imgsz=640
```

## GitHub Repo Description

`Zone-based RTSP/video dwell-time person detection with YOLO ONNX and OpenCV; saves alert snapshots and CSV logs.`
