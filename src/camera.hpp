#pragma once

#include <opencv2/opencv.hpp>
#include <string>

// Reads frames from RTSP, webcam index, or a video file.
// If the source drops, reconnect is attempted automatically.
class Camera {
public:
    explicit Camera(const std::string& source);
    ~Camera();

    bool open();
    bool read(cv::Mat& frame);
    bool readResized(cv::Mat& frame, int width, int height);
    bool reconnect();
    void release();
    bool isOpened() const;

    void setReconnectDelay(int delayMs);
    void setMaxFailedFrames(int maxFailedFrames);

    double getFPS() const;
    int getWidth() const;
    int getHeight() const;

private:
    std::string source_;
    cv::VideoCapture cap_;
    int failedFrameCount_ = 0;
    int maxFailedFrames_ = 30;
    int reconnectDelayMs_ = 1000;

    bool isRtsp() const;
    bool isCameraIndex() const;
    bool isVideoFile() const;
};
