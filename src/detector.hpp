#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

// A single detection result (only for the "person" class)
struct Detection {
    cv::Rect box;
    float confidence;
    cv::Point2f center;
};

// PersonDetector: loads a YOLOv8 ONNX model via OpenCV DNN and
// detects people (COCO class 0) in a frame.
class PersonDetector {
public:
    explicit PersonDetector(const std::string& modelPath);

    std::vector<Detection> detect(const cv::Mat& frame,
                                   float confThreshold = 0.35f,
                                   float nmsThreshold = 0.45f);

private:
    cv::dnn::Net net_;
    int inputWidth_ = 640;
    int inputHeight_ = 640;
    static constexpr int kPersonClassId = 0;

    std::vector<Detection> parseOutput(const cv::Mat& output,
                                        float scaleX, float scaleY,
                                        int origWidth, int origHeight,
                                        float confThreshold,
                                        float nmsThreshold);
};
