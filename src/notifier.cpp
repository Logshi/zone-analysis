#include "notifier.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

Notifier::Notifier(const std::string& alertDir) : alertDir_(alertDir) {
    csvPath_ = (fs::path(alertDir_) / "alerts.csv").string();
    ensureAlertDir();
    ensureCsvHeader();
}

void Notifier::ensureAlertDir() {
    if (!fs::exists(alertDir_)) {
        fs::create_directories(alertDir_);
        std::cout << "[Notifier] Alerts directory created: " << alertDir_ << std::endl;
    }
}

void Notifier::ensureCsvHeader() {
    if (!fs::exists(csvPath_)) {
        std::ofstream csv(csvPath_);
        csv << "timestamp,track_id,dwell_seconds,image_path\n";
        csv.close();
        std::cout << "[Notifier] CSV log file created: " << csvPath_ << std::endl;
    }
}

std::string Notifier::buildTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf;
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTime);
#else
    localtime_r(&nowTime, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void Notifier::sendAlert(int trackId, double dwellSeconds, const cv::Mat& frame, const cv::Rect& box) {
    std::string timestamp = buildTimestamp();

    cv::Mat snapshot = frame.clone();
    cv::rectangle(snapshot, box, cv::Scalar(0, 0, 255), 2);
    cv::putText(snapshot, "ALERT: 10s+ dwell", cv::Point(box.x, std::max(0, box.y - 10)),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);

    std::string imageName = "alert_" + timestamp + "_id" + std::to_string(trackId) + ".jpg";
    std::string imagePath = (fs::path(alertDir_) / imageName).string();

    cv::imwrite(imagePath, snapshot);

    std::ofstream csv(csvPath_, std::ios::app);
    csv << timestamp << "," << trackId << "," << dwellSeconds << "," << imagePath << "\n";
    csv.close();

    std::cout << "[ALARM] Track ID: " << trackId
              << " | Dwell: " << dwellSeconds << "s"
              << " | Snapshot: " << imagePath << std::endl;
}
