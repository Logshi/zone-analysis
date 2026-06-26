# models

This folder contains the ONNX model used by the C++ application.

Default model:

```text
models/yolo26n.onnx
```

You can replace it with another Ultralytics YOLO ONNX model exported at 640x640.

Example export:

```bash
pip install ultralytics
yolo export model=yolov8n.pt format=onnx imgsz=640
```
