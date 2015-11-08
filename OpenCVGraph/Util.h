#pragma once

#include "stdafx.h"
#include "opencv2/highgui.hpp"

struct CvWindow;

using namespace std;
using namespace cv;

bool fileExists(const std::string& name);

bool dirExists(const std::string& path);

void DrawShadowTextMono(cv::Mat m, string str, cv::Point p, double scale);

//bool getWindowRect(const String &  winname, cv::Rect2d rect)
//{
//    CvWindow* window;
//
//    window = icvFindWindowByName(name);
//}