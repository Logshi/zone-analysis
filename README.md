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

## Donanim Gereksinimleri

Bu uygulama varsayilan olarak OpenCV DNN `DNN_BACKEND_OPENCV` ve
`DNN_TARGET_CPU` ile calisir; bu nedenle GPU zorunlu degildir. Her frame
640x640 boyutuna yeniden olceklenir ve `models/yolo26n.onnx` modeli
calistirilir.

Arastirma notu:

- OpenCV DNN ONNX model yuklemeyi ve CPU hedefini destekler:
  https://docs.opencv.org/4.x/d6/d0f/group__dnn.html
- Ultralytics YOLO26n icin 640px benchmark degerleri: 2.4M parametre,
  5.4 GFLOPs, CPU ONNX 38.9 ms ve T4 TensorRT 1.7 ms:
  https://docs.ultralytics.com/models/yolo26/
- OpenCV Linux kurulumu icin CMake ve C++ derleyici gerektirir:
  https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html
- OpenCV Windows kurulumu icin CMake, Git ve Visual Studio/C++ derleyici
  akisindan bahseder:
  https://docs.opencv.org/4.x/d3/d52/tutorial_windows_install.html

Pratik minimum donanim:

| Bilesen | Minimum |
| --- | --- |
| CPU | 64-bit, 4 cekirdekli Intel i5 / AMD Ryzen 3 veya benzeri |
| RAM | 8 GB |
| GPU | Zorunlu degil; mevcut kod CPU ile calisir |
| Disk | Runtime icin 1 GB bos alan; Windows'ta vcpkg/OpenCV build icin 25-40 GB bos alan onerilir |
| Kamera/Ag | 1 adet RTSP kamera veya USB webcam; RTSP icin stabil LAN/Wi-Fi |
| Isletim sistemi | Windows 10/11 x64 veya Ubuntu/WSL 20.04+ |

Onerilen donanim:

| Kullanim | Oneri |
| --- | --- |
| Tek kamera, 640x640, dusuk/orta FPS | 6 cekirdekli modern CPU, 16 GB RAM |
| Tek kamera, daha akici izleme | Intel i5 10. nesil+ / Ryzen 5 3600+ veya benzeri |
| Coklu kamera veya yuksek FPS | 8+ cekirdek CPU, 16-32 GB RAM; GPU hizlandirma istenecekse kodun CUDA/TensorRT/OpenVINO icin ayrica uyarlanmasi gerekir |

Beklenen performans donanima, kamera cozunurlugune, RTSP codec'ine ve sahnedeki
kisi sayisina baglidir. YOLO26n CPU ONNX benchmarki sadece model inference
suresini gosterir; video decode, resize, takip, cizim ve ekran gosterimi toplam
FPS'i dusurebilir.

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
