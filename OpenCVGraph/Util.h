#pragma once

#include "stdafx.h"
#include "opencv2/highgui.hpp"

struct CvWindow;

using namespace std;
using namespace cv;

bool fileExists(const std::string& name);

bool dirExists(const std::string& path);

void DrawShadowTextMono(cv::Mat m, string str, cv::Point p, double scale);

int getU16Pix(const cv::Mat& img, cv::Point pt);

vector<Mat> createHistogramImages(Mat& img);