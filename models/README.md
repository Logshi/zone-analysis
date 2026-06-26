# models

This folder contains the ONNX model used by the C++ application.

Recommended default model:

```text
models/yolov8n.onnx
```

`models/yolo26n.onnx` is also included, but its attention (C2PSA) blocks use a
`Split` op that fails to import on older OpenCV DNN builds (e.g. OpenCV 4.5.x
on Ubuntu 22.04 and OpenCV 4.6.0 on Raspberry Pi OS bookworm), aborting the
app at startup. Use `yolov8n.onnx` unless you've verified your OpenCV build
supports it.

You can replace either with another Ultralytics YOLO ONNX model exported at
640x640.

Example export:

```bash
pip install ultralytics
yolo export model=yolov8n.pt format=onnx imgsz=640
```
