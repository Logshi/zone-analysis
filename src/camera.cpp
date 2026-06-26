#include "camera.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <thread>

Camera::Camera(const std::string& source) : source_(source) {}

Camera::~Camera() {
    release();
}

bool Camera::isRtsp() const {
    return source_.rfind("rtsp://", 0) == 0;
}

bool Camera::isCameraIndex() const {
    if (source_.empty()) {
        return false;
    }

    return std::all_of(source_.begin(), source_.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

bool Camera::isVideoFile() const {
    return !isRtsp() && !isCameraIndex();
}

bool Camera::open() {
    if (isRtsp()) {
        cap_.open(source_, cv::CAP_FFMPEG);
    } else if (isCameraIndex()) {
        cap_.open(std::stoi(source_));
    } else {
        cap_.open(source_);
    }

    if (!cap_.isOpened()) {
        std::cerr << "[Camera] Failed to open source: " << source_ << std::endl;
        return false;
    }

    failedFrameCount_ = 0;
    std::cout << "[Camera] Source opened: " << source_ << std::endl;
    return true;
}

bool Camera::read(cv::Mat& frame) {
    if (!cap_.isOpened()) {
        return false;
    }

    bool ok = cap_.read(frame);
    if ((!ok || frame.empty()) && isVideoFile()) {
        // Loop video files: end-of-file is not an error, just rewind and keep playing
        cap_.set(cv::CAP_PROP_POS_FRAMES, 0);
        ok = cap_.read(frame);
    }

    if (!ok || frame.empty()) {
        failedFrameCount_++;
        std::cerr << "[Camera] Failed to read frame (" << failedFrameCount_
                  << "/" << maxFailedFrames_ << ")" << std::endl;

        if (failedFrameCount_ >= maxFailedFrames_) {
            std::cerr << "[Camera] Max failed frame count reached, reconnecting..." << std::endl;
            reconnect();
        }
        return false;
    }

    failedFrameCount_ = 0;
    return true;
}

bool Camera::readResized(cv::Mat& frame, int width, int height) {
    cv::Mat raw;
    if (!read(raw)) {
        return false;
    }
    cv::resize(raw, frame, cv::Size(width, height));
    return true;
}

bool Camera::reconnect() {
    release();
    std::this_thread::sleep_for(std::chrono::milliseconds(reconnectDelayMs_));
    return open();
}

void Camera::release() {
    if (cap_.isOpened()) {
        cap_.release();
    }
}

bool Camera::isOpened() const {
    return cap_.isOpened();
}

void Camera::setReconnectDelay(int delayMs) {
    reconnectDelayMs_ = std::max(0, delayMs);
}

void Camera::setMaxFailedFrames(int maxFailedFrames) {
    maxFailedFrames_ = std::max(1, maxFailedFrames);
}

double Camera::getFPS() const {
    return cap_.get(cv::CAP_PROP_FPS);
}

int Camera::getWidth() const {
    return static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
}

int Camera::getHeight() const {
    return static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
}
