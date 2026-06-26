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
            throw std::runtime_error("Region noktasi gecersiz: " + token);
        }

        int x = std::stoi(token.substr(0, comma));
        int y = std::stoi(token.substr(comma + 1));
        points.emplace_back(x, y);
    }

    if (points.size() < 3) {
        throw std::runtime_error("Region en az 3 nokta icermelidir.");
    }

    return points;
}

void printUsage(const char* executable) {
    std::cerr << "Kullanim: " << executable
              << " <rtsp_url_veya_video_path_veya_webcam_index> <onnx_model_path>"
              << " [--dwell seconds] [--region \"x1,y1;x2,y2;x3,y3\"] [--alerts alerts_dir] [--no-gui]"
              << std::endl;
    std::cerr << "Ornek (RTSP) : " << executable
              << " \"rtsp://admin:password@192.168.1.50:554/stream1\" models/yolo26n.onnx"
              << std::endl;
    std::cerr << "Ornek (video): " << executable
              << " test.mp4 models/yolo26n.onnx --dwell 15"
              << std::endl;
    std::cerr << "Ornek (webcam): " << executable
              << " 0 models/yolo26n.onnx --region \"180,200;500,200;560,560;120,560\""
              << std::endl;
}

struct Options {
    std::string source;
    std::string modelPath;
    double dwellSeconds = 10.0;
    std::vector<cv::Point> region = defaultRegion();
    bool regionSet = false;
    std::string alertsDir = "alerts";
    bool gui = true;
};

Options parseArgs(int argc, char** argv) {
    if (argc < 3) {
        throw std::runtime_error("Eksik arguman.");
    }

    Options options;
    options.source = argv[1];
    options.modelPath = argv[2];

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];

        auto requireValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(name + " icin deger gerekli.");
            }
            return argv[++i];
        };

        if (arg == "--dwell") {
            options.dwellSeconds = std::stod(requireValue(arg));
            if (options.dwellSeconds <= 0.0) {
                throw std::runtime_error("--dwell pozitif olmalidir.");
            }
        } else if (arg == "--region") {
            options.region = parseRegion(requireValue(arg));
            options.regionSet = true;
        } else if (arg == "--alerts") {
            options.alertsDir = requireValue(arg);
        } else if (arg == "--no-gui") {
            options.gui = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        } else {
            throw std::runtime_error("Bilinmeyen arguman: " + arg);
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

// Mouse ile manuel polygon cizimi icin durum tutar
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

// Kullanicinin canli goruntu uzerinde fare ile polygon cizmesini saglar.
// Sol tik: nokta ekler, sag tik: son noktayi siler, 'c': onayla, 'r': sifirla, ESC: varsayilan bolge.
std::vector<cv::Point> drawRegionInteractive(Camera& camera, const std::string& windowName,
                                              int width, int height) {
    PolygonDrawState state;
    cv::setMouseCallback(windowName, onPolygonMouse, &state);

    std::cout << "[main] Polygon cizim modu: sol tik nokta ekler, sag tik son noktayi siler." << std::endl;
    std::cout << "[main] 'c' ile onayla, 'r' ile sifirla, ESC ile varsayilan bolgeyi kullan." << std::endl;

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

        cv::putText(display, "Polygon ciz: sol=ekle sag=geri al  c=onayla r=sifirla ESC=varsayilan",
                    cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 255), 2);

        cv::imshow(windowName, display);

        int key = cv::waitKey(30);
        if (key == 'c' || key == 'C') {
            if (state.points.size() >= 3) {
                result = state.points;
                break;
            }
            std::cout << "[main] En az 3 nokta gerekli." << std::endl;
        } else if (key == 'r' || key == 'R') {
            state.points.clear();
        } else if (key == 27) { // ESC
            std::cout << "[main] Varsayilan polygon kullanilacak." << std::endl;
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
        std::cerr << "[main] Kamera/video acilamadi, cikiliyor." << std::endl;
        return 1;
    }

    PersonDetector detector(options.modelPath);
    DwellTracker tracker;
    tracker.setDwellLimitSeconds(options.dwellSeconds);
    Notifier notifier(options.alertsDir);

    if (options.gui) {
        std::cout << "[main] Sistem baslatildi. Cikis icin 'q' veya ESC." << std::endl;
    } else {
        std::cout << "[main] Sistem baslatildi. Headless modda cikis icin Ctrl+C." << std::endl;
    }
    std::cout << "[main] Dwell limit: " << options.dwellSeconds << "s"
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

        std::vector<Detection> detections = detector.detect(frame);
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
            std::cout << "[main] Cikis yapiliyor." << std::endl;
            break;
        }
    }

    camera.release();
    if (options.gui) {
        cv::destroyAllWindows();
    }
    return 0;
}
