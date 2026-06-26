#pragma once

#include <opencv2/opencv.hpp>
#include <string>

// Notifier: alarm olustugunda snapshot kaydeder, CSV log atar
// ve terminale bilgi yazdirir.
class Notifier {
public:
    explicit Notifier(const std::string& alertDir);

    void sendAlert(int trackId, double dwellSeconds, const cv::Mat& frame, const cv::Rect& box);

private:
    std::string alertDir_;
    std::string csvPath_;

    void ensureAlertDir();
    void ensureCsvHeader();
    std::string buildTimestamp() const;
};
