#pragma once

#include "stdafx.h"

using namespace std;
using namespace cv;

bool fileExists(const std::string& name);

bool dirExists(const std::string& path);

void DrawShadowTextMono(cv::Mat m, string str, cv::Point p, double scale);