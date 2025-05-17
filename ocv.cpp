#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char** argv) {
    // Validate input
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <template_image_path>\n";
        return -1;
    }

    // 1. Load template in grayscale
    cv::Mat templ = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    if (templ.empty()) {
        std::cerr << "Error: Cannot load template image " << argv[1] << "\n";
        return -1;
    }  // 

    // 2. Open default camera (index 0)
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open camera\n";
        return -1;
    }  // 

    // 3. Window for display
    const std::string winName = "Template Matching";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::Mat frame, gray, result;
    while (true) {
        cap >> frame;                        // Capture a frame :contentReference[oaicite:3]{index=3}
        if (frame.empty()) break;

        // Convert to grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);  // 

        // Prepare result matrix of correct size
        int result_cols = gray.cols - templ.cols + 1;
        int result_rows = gray.rows - templ.rows + 1;
        result.create(result_rows, result_cols, CV_32FC1);

        // 4. Template matching
        cv::matchTemplate(gray, templ, result, cv::TM_CCOEFF_NORMED);  // :contentReference[oaicite:4]{index=4}

        // 5. Find best match location
        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);   // :contentReference[oaicite:5]{index=5}

        cv::Point matchLoc = maxLoc;  // For TM_CCOEFF_NORMED, maxLoc is best :contentReference[oaicite:6]{index=6}

        // 6. Draw rectangle around the matched region
        cv::rectangle(
            frame,
            matchLoc,
            cv::Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows),
            cv::Scalar(0, 255, 0),
            2
        );  // :contentReference[oaicite:7]{index=7}

        // 7. Display
        cv::imshow(winName, frame);
        if (cv::waitKey(30) == 27) break;  // Exit on ESC :contentReference[oaicite:8]{index=8}
    }

    // Cleanup
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
