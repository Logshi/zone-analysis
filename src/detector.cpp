#include "detector.hpp"
#include <iostream>

PersonDetector::PersonDetector(const std::string& modelPath) {
    net_ = cv::dnn::readNetFromONNX(modelPath);
    if (net_.empty()) {
        std::cerr << "[PersonDetector] Failed to load model: " << modelPath << std::endl;
        throw std::runtime_error("Failed to load model: " + modelPath);
    }

    // CPU backend is used; can be swapped here if a GPU is available
    net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    std::cout << "[PersonDetector] Model loaded: " << modelPath << std::endl;
}

std::vector<Detection> PersonDetector::detect(const cv::Mat& frame,
                                               float confThreshold,
                                               float nmsThreshold) {
    std::vector<Detection> results;
    if (frame.empty()) {
        return results;
    }

    int origWidth = frame.cols;
    int origHeight = frame.rows;

    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0,
                            cv::Size(inputWidth_, inputHeight_),
                            cv::Scalar(), true, false);
    net_.setInput(blob);

    std::vector<cv::Mat> outputs;
    net_.forward(outputs, net_.getUnconnectedOutLayersNames());

    if (outputs.empty()) {
        std::cerr << "[PersonDetector] Model output is empty!" << std::endl;
        return results;
    }

    cv::Mat output = outputs[0];

    float scaleX = static_cast<float>(origWidth) / static_cast<float>(inputWidth_);
    float scaleY = static_cast<float>(origHeight) / static_cast<float>(inputHeight_);

    return parseOutput(output, scaleX, scaleY, origWidth, origHeight,
                        confThreshold, nmsThreshold);
}

std::vector<Detection> PersonDetector::parseOutput(const cv::Mat& output,
                                                    float scaleX, float scaleY,
                                                    int origWidth, int origHeight,
                                                    float confThreshold,
                                                    float nmsThreshold) {
    std::vector<Detection> results;

    if (output.dims != 3) {
        std::cerr << "[PersonDetector] Unexpected model output rank (dims="
                  << output.dims << "). Expected [1,84,8400] or [1,8400,84]." << std::endl;
        return results;
    }

    int d1 = output.size[1];
    int d2 = output.size[2];

    // Expected channel count: 4 (box) + 80 (class) = 84
    cv::Mat data; // [numDetections x 84]

    if (d1 == 84 && d2 != 84) {
        // [1, 84, 8400] -> transpose to [8400, 84]
        cv::Mat reshaped = output.reshape(1, d1); // 84 x 8400
        cv::transpose(reshaped, data);
    } else if (d2 == 84 && d1 != 84) {
        // [1, 8400, 84] -> use directly
        data = output.reshape(1, d1); // 8400 x 84
    } else {
        std::cerr << "[PersonDetector] Unexpected model output shape: ["
                  << d1 << ", " << d2 << "]. Expected [1,84,8400] or [1,8400,84]." << std::endl;
        return results;
    }

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;

    int numDetections = data.rows;
    int numClasses = data.cols - 4;

    for (int i = 0; i < numDetections; ++i) {
        const float* row = data.ptr<float>(i);

        float cx = row[0];
        float cy = row[1];
        float w = row[2];
        float h = row[3];

        // Find the highest-scoring class; only accept the detection if it is person (class 0)
        int bestClassId = 0;
        float bestScore = row[4];
        for (int c = 1; c < numClasses; ++c) {
            float score = row[4 + c];
            if (score > bestScore) {
                bestScore = score;
                bestClassId = c;
            }
        }

        if (bestClassId != kPersonClassId) {
            continue;
        }

        if (bestScore < confThreshold) {
            continue;
        }

        float left = (cx - w / 2.0f) * scaleX;
        float top = (cy - h / 2.0f) * scaleY;
        float boxWidth = w * scaleX;
        float boxHeight = h * scaleY;

        cv::Rect box(static_cast<int>(left), static_cast<int>(top),
                     static_cast<int>(boxWidth), static_cast<int>(boxHeight));

        // Clamp boxes that extend past the frame edges
        box &= cv::Rect(0, 0, origWidth, origHeight);
        if (box.width <= 0 || box.height <= 0) {
            continue;
        }

        boxes.push_back(box);
        confidences.push_back(bestScore);
    }

    std::vector<int> nmsIndices;
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, nmsIndices);

    for (int idx : nmsIndices) {
        Detection det;
        det.box = boxes[idx];
        det.confidence = confidences[idx];
        det.center = cv::Point2f(det.box.x + det.box.width / 2.0f,
                                  det.box.y + det.box.height / 2.0f);
        results.push_back(det);
    }

    return results;
}
