#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "camera.hpp"
#include "detector.hpp"
#include "notifier.hpp"
#include "tracker.hpp"

namespace {

constexpr int kInputWidth = 640;
constexpr int kInputHeight = 640;

std::vector<cv::Point> defaultRegion() {
    return {
        cv::Point(180, 200),
        cv::Point(500, 200),
        cv::Point(560, 560),
        cv::Point(120, 560)
    };
}

std::vector<cv::Point> parseRegion(const std::string& value) {
    std::vector<cv::Point> points;
    std::stringstream pointStream(value);
    std::string token;

    while (std::getline(pointStream, token, ';')) {
        std::size_t comma = token.find(',');
        if (comma == std::string::npos) {
            throw std::runtime_error("Invalid region point: " + token);
        }

        int x = std::stoi(token.substr(0, comma));
        int y = std::stoi(token.substr(comma + 1));
        points.emplace_back(x, y);
    }

    if (points.size() < 3) {
        throw std::runtime_error("Region must contain at least 3 points.");
    }

    return points;
}

void printUsage(const char* executable) {
    std::cerr << "Usage: " << executable
              << " <rtsp_url_or_video_path_or_webcam_index> <onnx_model_path>"
              << " [--dwell seconds] [--region \"x1,y1;x2,y2;x3,y3\"] [--alerts alerts_dir]"
              << " [--conf threshold] [--no-gui]"
              << std::endl;
    std::cerr << "Example (RTSP)  : " << executable
              << " \"rtsp://admin:password@192.168.1.50:554/stream1\" models/yolov8n.onnx"
              << std::endl;
    std::cerr << "Example (video) : " << executable
              << " test.mp4 models/yolov8n.onnx --dwell 15 --conf 0.6"
              << std::endl;
    std::cerr << "Example (webcam): " << executable
              << " 0 models/yolov8n.onnx --region \"180,200;500,200;560,560;120,560\""
              << std::endl;
}

struct Options {
    std::string source;
    std::string modelPath;
    double dwellSeconds = 10.0;
    std::vector<cv::Point> region = defaultRegion();
    bool regionSet = false;
    std::string alertsDir = "alerts";
    float confThreshold = 0.5f;
    bool gui = true;
};

Options parseArgs(int argc, char** argv) {
    if (argc < 3) {
        throw std::runtime_error("Missing arguments.");
    }

    Options options;
    options.source = argv[1];
    options.modelPath = argv[2];

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];

        auto requireValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(name + " requires a value.");
            }
            return argv[++i];
        };

        if (arg == "--dwell") {
            options.dwellSeconds = std::stod(requireValue(arg));
            if (options.dwellSeconds <= 0.0) {
                throw std::runtime_error("--dwell must be positive.");
            }
        } else if (arg == "--region") {
            options.region = parseRegion(requireValue(arg));
            options.regionSet = true;
        } else if (arg == "--alerts") {
            options.alertsDir = requireValue(arg);
        } else if (arg == "--conf") {
            options.confThreshold = std::stof(requireValue(arg));
            if (options.confThreshold <= 0.0f || options.confThreshold >= 1.0f) {
                throw std::runtime_error("--conf must be between 0 and 1.");
            }
        } else if (arg == "--no-gui") {
            options.gui = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    return options;
}

void drawRegion(cv::Mat& frame, const std::vector<cv::Point>& region) {
    std::vector<std::vector<cv::Point>> contours{region};
    cv::polylines(frame, contours, true, cv::Scalar(255, 0, 0), 2);
}

void drawTracks(cv::Mat& frame, const std::unordered_map<int, Track>& tracks) {
    Clock::time_point now = Clock::now();

    for (const auto& pair : tracks) {
        const Track& track = pair.second;
        cv::Scalar color = track.inside ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);

        cv::rectangle(frame, track.box, color, 2);

        std::string label = "ID " + std::to_string(track.id);
        if (track.inside) {
            double dwell = std::chrono::duration<double>(now - track.enteredAt).count();
            std::ostringstream oss;
            oss << " " << std::fixed << std::setprecision(1) << dwell << "s";
            label += oss.str();
        }

        cv::putText(frame, label,
                    cv::Point(track.box.x, std::max(0, track.box.y - 8)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
    }
}

// Holds state for drawing the polygon with the mouse
struct PolygonDrawState {
    std::vector<cv::Point> points;
};

void onPolygonMouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* state = static_cast<PolygonDrawState*>(userdata);
    if (event == cv::EVENT_LBUTTONDOWN) {
        state->points.emplace_back(x, y);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        if (!state->points.empty()) {
            state->points.pop_back();
        }
    }
}

// Lets the user draw the polygon region with the mouse on the live feed.
// Left click: add point, right click: remove last point, 'c': confirm, 'r': reset, ESC: use default region.
std::vector<cv::Point> drawRegionInteractive(Camera& camera, const std::string& windowName,
                                              int width, int height) {
    PolygonDrawState state;
    cv::setMouseCallback(windowName, onPolygonMouse, &state);

    std::cout << "[main] Polygon draw mode: left click adds a point, right click removes the last one." << std::endl;
    std::cout << "[main] Press 'c' to confirm, 'r' to reset, ESC to use the default region." << std::endl;

    cv::Mat frame;
    std::vector<cv::Point> result;

    while (true) {
        if (!camera.readResized(frame, width, height)) {
            continue;
        }

        cv::Mat display = frame.clone();

        for (size_t i = 0; i < state.points.size(); ++i) {
            cv::circle(display, state.points[i], 4, cv::Scalar(0, 255, 255), -1);
            if (i > 0) {
                cv::line(display, state.points[i - 1], state.points[i], cv::Scalar(255, 0, 0), 2);
            }
        }
        if (state.points.size() >= 3) {
            cv::line(display, state.points.back(), state.points.front(), cv::Scalar(255, 0, 0), 1);
        }

        cv::putText(display, "Draw polygon: left=add right=undo  c=confirm r=reset ESC=default",
                    cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 255), 2);

        cv::imshow(windowName, display);

        int key = cv::waitKey(30);
        if (key == 'c' || key == 'C') {
            if (state.points.size() >= 3) {
                result = state.points;
                break;
            }
            std::cout << "[main] At least 3 points are required." << std::endl;
        } else if (key == 'r' || key == 'R') {
            state.points.clear();
        } else if (key == 27) { // ESC
            std::cout << "[main] Using the default region." << std::endl;
            result = defaultRegion();
            break;
        }
    }

    cv::setMouseCallback(windowName, nullptr, nullptr);
    return result;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    try {
        options = parseArgs(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "[main] " << ex.what() << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    Camera camera(options.source);
    if (!camera.open()) {
        std::cerr << "[main] Failed to open camera/video, exiting." << std::endl;
        return 1;
    }

    PersonDetector detector(options.modelPath);
    DwellTracker tracker;
    tracker.setDwellLimitSeconds(options.dwellSeconds);
    Notifier notifier(options.alertsDir);

    if (options.gui) {
        std::cout << "[main] System started. Press 'q' or ESC to quit." << std::endl;
    } else {
        std::cout << "[main] System started. Press Ctrl+C to quit in headless mode." << std::endl;
    }
    std::cout << "[main] Dwell limit: " << options.dwellSeconds << "s"
              << " | Confidence: " << options.confThreshold
              << " | Alerts: " << options.alertsDir
              << " | GUI: " << (options.gui ? "on" : "off") << std::endl;

    const std::string windowName = "RTSP Dwell Alert";
    if (options.gui) {
        cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);

        if (!options.regionSet) {
            options.region = drawRegionInteractive(camera, windowName, kInputWidth, kInputHeight);
        }
    }

    cv::Mat frame;
    while (true) {
        if (!camera.readResized(frame, kInputWidth, kInputHeight)) {
            continue;
        }

        std::vector<Detection> detections;
        try {
            detections = detector.detect(frame, options.confThreshold);
        } catch (const cv::Exception& ex) {
            // A frame corrupted by RTSP packet loss can crash the DNN forward pass;
            // skip detection for this frame and keep going.
            std::cerr << "[main] Detection error, skipping frame: " << ex.what() << std::endl;
            continue;
        }

        AlertEvent alert = tracker.update(detections, options.region);

        if (alert.hasAlert) {
            notifier.sendAlert(alert.trackId, alert.dwellSeconds, frame, alert.box);
        }

        if (!options.gui) {
            continue;
        }

        drawRegion(frame, options.region);
        drawTracks(frame, tracker.getTracks());
        cv::imshow(windowName, frame);

        int key = cv::waitKey(1);
        if (key == 'q' || key == 27) {
            std::cout << "[main] Exiting." << std::endl;
            break;
        }
    }

    camera.release();
    if (options.gui) {
        cv::destroyAllWindows();
    }
    return 0;
}
