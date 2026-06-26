import argparse
from pathlib import Path

import cv2
from ultralytics import YOLO


def parse_args():
    parser = argparse.ArgumentParser(description="Run a quick ONNX detection test on one image.")
    parser.add_argument("--model", default="models/yolo26n.onnx", help="Path to ONNX model.")
    parser.add_argument("--image", required=True, help="Path to test image.")
    parser.add_argument("--save", default="", help="Optional output directory for rendered result.")
    parser.add_argument("--show", action="store_true", help="Show result in an OpenCV window.")
    return parser.parse_args()


def main():
    args = parse_args()
    model = YOLO(args.model, task="detect")
    results = model(args.image)

    for index, result in enumerate(results):
        rendered = result.plot()

        if args.save:
            output_dir = Path(args.save)
            output_dir.mkdir(parents=True, exist_ok=True)
            output_path = output_dir / f"onnx_test_{index}.jpg"
            cv2.imwrite(str(output_path), rendered)
            print(f"Saved: {output_path}")

        if args.show:
            cv2.imshow("YOLO ONNX Result", rendered)
            cv2.waitKey(0)


if __name__ == "__main__":
    main()
