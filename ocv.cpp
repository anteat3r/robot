#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>

int main() {
    // 1. Open video capture (camera or file)
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: cannot open camera\n";
        return -1;
    }  // :contentReference[oaicite:4]{index=4}

    // 2. Read first frame and select ROI
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        std::cerr << "Error: blank frame grabbed\n";
        return -1;
    }

    // Let user select the object to track
    cv::Rect2d roi = cv::selectROI("Select Object", frame, false, false);
    if (roi.width == 0 || roi.height == 0) {
        std::cerr << "Error: no ROI selected\n";
        return -1;
    }  // :contentReference[oaicite:5]{index=5}

    // 3. Create CSRT tracker and initialize
    cv::Ptr<cv::Tracker> tracker = cv::TrackerCSRT::create();
    tracker->init(frame, roi);  // :contentReference[oaicite:6]{index=6}

    // 4. Tracking loop
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // Update tracker and get new bounding box
        bool ok = tracker->update(frame, roi);
        if (ok) {
            // Draw tracked box
            cv::rectangle(frame, roi, cv::Scalar(0, 255, 0), 2, 1);
        } else {
            // Tracking failure
            cv::putText(frame, "Tracking failure", {50, 80},
                        cv::FONT_HERSHEY_SIMPLEX, 0.75, {0, 0, 255}, 2);
        }  // :contentReference[oaicite:7]{index=7}

        // Display FPS and instructions
        cv::putText(frame, "CSRT Tracker", {20, 20},
                    cv::FONT_HERSHEY_SIMPLEX, 0.75, {255, 255, 255}, 2);
        cv::imshow("Tracking", frame);

        // Exit on ESC
        if (cv::waitKey(30) == 27) break;  // :contentReference[oaicite:8]{index=8}
    }

    return 0;
}
