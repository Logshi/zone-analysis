#pragma once

#include <opencv2/opencv.hpp>
#include <chrono>
#include <unordered_map>
#include <vector>
#include "detector.hpp"

using Clock = std::chrono::steady_clock;

// State of a single tracked person
struct Track {
    int id;
    cv::Point2f center;
    cv::Rect box;
    bool inside = false;
    bool notified = false;
    Clock::time_point enteredAt;
    Clock::time_point lastSeen;
};

// Returned when a dwell-time alert fires inside the polygon region
struct AlertEvent {
    bool hasAlert = false;
    int trackId = -1;
    double dwellSeconds = 0.0;
    cv::Rect box;
};

// DwellTracker: does simple center-distance based tracking and raises
// an alert for anyone who stays inside the polygon region too long.
class DwellTracker {
public:
    DwellTracker();

    AlertEvent update(const std::vector<Detection>& detections,
                       const std::vector<cv::Point>& region);

    const std::unordered_map<int, Track>& getTracks() const;

    void setDwellLimitSeconds(double seconds);
    void setMaxAssignDistance(float distance);
    void setMaxUnseenSeconds(double seconds);

private:
    std::unordered_map<int, Track> tracks_;
    int nextId_ = 0;

    double dwellLimitSeconds_ = 10.0;
    float maxAssignDistance_ = 90.0f;
    double maxUnseenSeconds_ = 3.0;

    bool isInsideRegion(const cv::Point2f& point, const std::vector<cv::Point>& region) const;
};
