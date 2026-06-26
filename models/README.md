# models

This folder holds the ONNX model used by the C++ application. **No model
file is committed to this repo** — Ultralytics YOLO weights (including
YOLOv8/YOLO11/YOLO26) are licensed under AGPL-3.0, which has redistribution
obligations; to keep this repo's own license simple and avoid any licensing
ambiguity, export or download the model yourself and drop it here.

Recommended model: `yolov8n.onnx`, exported at 640x640. Note that older
OpenCV DNN builds (e.g. OpenCV 4.5.x on Ubuntu 22.04 and OpenCV 4.6.0 on
Raspberry Pi OS bookworm) fail to run newer YOLO architectures with
attention (C2PSA) blocks — see the main README's Raspberry Pi section for
the OpenCV upgrade needed to run those models. `yolov8n` itself has no
attention blocks and works fine on those older builds too, as long as the
export command below is used.

Export with Ultralytics (requires accepting the AGPL-3.0 license, or a
commercial Ultralytics license for closed-source use):

```bash
pip install ultralytics
yolo export model=yolov8n.pt format=onnx imgsz=640
```

Then place the resulting `yolov8n.onnx` in this folder and point the app at
it with the second CLI argument, e.g. `models/yolov8n.onnx`.
