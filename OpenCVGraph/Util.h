#pragma once

#include "stdafx.h"
#include "opencv2/highgui.hpp"

using namespace std;
using namespace cv;

//namespace openCVGraph
//{
    bool fileExists(const std::string& name);

    bool dirExists(const std::string& path);

    void createDir(std::string& dir);

    void DrawOverlayText(cv::Mat m, string str, cv::Point p, double scale, CvScalar color = CV_RGB(255, 255, 255));

    int getU16Pix(const cv::Mat& img, cv::Point pt);

    Mat createGrayHistogram(Mat& img, int bins, int width = 256, int height = 400);
//}
