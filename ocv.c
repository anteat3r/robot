#include <opencv2/cv.h>
#include <opencv2/highgui.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <template_image>\n", argv[0]);
        return -1;
    }

    // 1. Load the template image (in grayscale)
    IplImage* templ = cvLoadImage(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    if (!templ) {
        fprintf(stderr, "Error: could not load template %s\n", argv[1]);
        return -1;
    }

    // 2. Open camera 0
    CvCapture* cap = cvCaptureFromCAM(0);
    if (!cap) {
        fprintf(stderr, "Error: could not open camera\n");
        cvReleaseImage(&templ);
        return -1;
    }

    // 3. Prepare windows
    cvNamedWindow("Live", CV_WINDOW_AUTOSIZE);

    // Pre-allocate result image
    CvSize frame_size = cvGetSize(cvQueryFrame(cap));
    CvSize result_size = cvSize(
        frame_size.width  - templ->width  + 1,
        frame_size.height - templ->height + 1
    );
    IplImage* result = cvCreateImage(result_size, IPL_DEPTH_32F, 1);

    while (1) {
        // 4a. Grab new frame, convert to grayscale
        IplImage* frame = cvQueryFrame(cap);
        if (!frame) break;
        IplImage* gray = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
        cvCvtColor(frame, gray, CV_BGR2GRAY);

        // 4b. Match template
        cvMatchTemplate(gray, templ, result, CV_TM_CCOEFF_NORMED);
        // cvMatchTemplate slides the template over the image and computes a similarity map :contentReference[oaicite:0]{index=0}

        // 4c. Find best match location
        double minVal, maxVal;
        CvPoint minLoc, maxLoc;
        cvMinMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, NULL);
        // For TM_CCOEFF_NORMED, the maximum gives the best match :contentReference[oaicite:1]{index=1}
        CvPoint matchLoc = maxLoc;

        // 4d. Draw rectangle around match
        cvRectangle(frame,
                    matchLoc,
                    cvPoint(matchLoc.x + templ->width,
                            matchLoc.y + templ->height),
                    CV_RGB(0,255,0), 2, 8, 0);

        // 4e. Show result
        cvShowImage("Live", frame);
        cvReleaseImage(&gray);

        // Exit on ESC
        if (cvWaitKey(30) == 27) break;
    }

    // 5. Cleanup
    cvReleaseImage(&templ);
    cvReleaseImage(&result);
    cvReleaseCapture(&cap);
    cvDestroyAllWindows();
    return 0;
}
