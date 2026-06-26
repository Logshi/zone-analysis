#include "tracker.hpp"
#include <limits>

DwellTracker::DwellTracker() {}

bool DwellTracker::isInsideRegion(const cv::Point2f& point,
                                   const std::vector<cv::Point>& region) const {
    if (region.size() < 3) {
        return false;
    }
    double result = cv::pointPolygonTest(region, point, false);
    return result >= 0;
}

AlertEvent DwellTracker::update(const std::vector<Detection>& detections,
                                 const std::vector<cv::Point>& region) {
    AlertEvent alertEvent;
    Clock::time_point now = Clock::now();

    std::vector<bool> used(detections.size(), false);

    // 1. Match existing tracks to the nearest detection
    for (auto& pair : tracks_) {
        Track& track = pair.second;

        int bestIdx = -1;
        float bestDist = std::numeric_limits<float>::max();

        for (size_t i = 0; i < detections.size(); ++i) {
            if (used[i]) continue;
            float dist = static_cast<float>(cv::norm(track.center - detections[i].center));
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = static_cast<int>(i);
            }
        }

        if (bestIdx != -1 && bestDist <= maxAssignDistance_) {
            track.center = detections[bestIdx].center;
            track.box = detections[bestIdx].box;
            track.lastSeen = now;
            used[bestIdx] = true;
        }
    }

    // 2. Create a new track for unmatched detections
    for (size_t i = 0; i < detections.size(); ++i) {
        if (used[i]) continue;

        Track track;
        track.id = nextId_++;
        track.center = detections[i].center;
        track.box = detections[i].box;
        track.inside = false;
        track.notified = false;
        track.lastSeen = now;
        tracks_[track.id] = track;
    }

    // 3. Update inside-region state and the dwell-time alert
    for (auto& pair : tracks_) {
        Track& track = pair.second;
        bool insideNow = isInsideRegion(track.center, region);

        if (insideNow && !track.inside) {
            // Just entered the region
            track.enteredAt = now;
            track.notified = false;
        } else if (!insideNow && track.inside) {
            // Left the region, reset the counter
            track.notified = false;
        }
        track.inside = insideNow;

        if (insideNow && !track.notified) {
            double dwell = std::chrono::duration<double>(now - track.enteredAt).count();
            if (dwell >= dwellLimitSeconds_) {
                track.notified = true;
                if (!alertEvent.hasAlert) {
                    alertEvent.hasAlert = true;
                    alertEvent.trackId = track.id;
                    alertEvent.dwellSeconds = dwell;
                    alertEvent.box = track.box;
                }
            }
        }
    }

    // 4. Remove tracks that haven't been seen for a while
    for (auto it = tracks_.begin(); it != tracks_.end(); ) {
        double unseen = std::chrono::duration<double>(now - it->second.lastSeen).count();
        if (unseen > maxUnseenSeconds_) {
            it = tracks_.erase(it);
        } else {
            ++it;
        }
    }

    return alertEvent;
}

const std::unordered_map<int, Track>& DwellTracker::getTracks() const {
    return tracks_;
}

void DwellTracker::setDwellLimitSeconds(double seconds) {
    dwellLimitSeconds_ = seconds;
}

void DwellTracker::setMaxAssignDistance(float distance) {
    maxAssignDistance_ = distance;
}

void DwellTracker::setMaxUnseenSeconds(double seconds) {
    maxUnseenSeconds_ = seconds;
}
