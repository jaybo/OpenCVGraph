#pragma once

#ifndef INCLUDE_OCVG_UTIL_HPP
#define INCLUDE_OCVG_UTIL_HPP

#include "stdafx.h"
#include "opencv2/highgui.hpp"

using namespace std;
using namespace cv;

//namespace openCVGraph
//{
    bool fileExists(const std::string& name);

    bool dirExists(const std::string& path);

    void createDir(std::string& dir);
    // void createDir();

    void DrawOverlayTextMono(cv::Mat m, string str, cv::Point p, double scale);

    int getU16Pix(const cv::Mat& img, cv::Point pt);

    Mat createGrayHistogram(Mat& img, int bins, int width = 256, int height = 400);
//}
#endif