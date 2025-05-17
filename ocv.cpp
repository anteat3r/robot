#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char** argv) {
    // Check for proper usage
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <template_image>" << std::endl;
        return -1;
    }

    // Load the template image in grayscale
    cv::Mat templ = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    if (templ.empty()) {
        std::cerr << "Error: Could not load template image " << argv[1] << std::endl;
        return -1;
    }

    // Open the default camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return -1;
    }

    // Create a window to display results
    cv::namedWindow("Template Matching", cv::WINDOW_AUTOSIZE);

    while (true) {
        cv::Mat frame, gray;
        cap >> frame; // Capture a new frame
        if (frame.empty()) break;

        // Convert the frame to grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Create the result matrix
        int result_cols = gray.cols - templ.cols + 1;
        int result_rows = gray.rows - templ.rows + 1;
        cv::Mat result(result_rows, result_cols, CV_32FC1);

        // Perform template matching
        cv::matchTemplate(gray, templ, result, cv::TM_CCOEFF_NORMED);

        // Localize the best match with minMaxLoc
        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

        // Define the top-left point of the matching area
        cv::Point matchLoc = maxLoc;

        // Draw a rectangle around the matched region
        cv::rectangle(frame, matchLoc, cv::Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), cv::Scalar(0, 255, 0), 2);

        // Display the result
        cv::imshow("Template Matching", frame);

        // Exit the loop if the ESC key is pressed
        if (cv::waitKey(30) == 27) break;
    }

    // Release resources
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
