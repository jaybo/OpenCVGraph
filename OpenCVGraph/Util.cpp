#include "stdafx.h"

using namespace std;

bool fileExists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool dirExists(const std::string& path)
{
    struct stat info;

    if (stat(path.c_str(), &info) != 0)
        return false;
    else if (info.st_mode & S_IFDIR)
        return true;
    else
        return false;
}

void DrawShadowTextMono(cv::Mat m, string str, cv::Point p, double scale)
{
    int offset = 3;
    cv::Point ptOffset = p;
    ptOffset.x += 2;
    ptOffset.y += 2;
    int depth = m.depth();
    double mx = 255;
    if (depth <= 1) {   // 8bit
        mx = 255;
    }
    else if (depth <= 3) { // 16bit
        mx = 65535;
    }
    cv::putText(m, str, ptOffset, CV_FONT_HERSHEY_DUPLEX, scale, CV_RGB(0, 0, 0));
    cv::putText(m, str, p, CV_FONT_HERSHEY_DUPLEX, scale, CV_RGB(mx, mx, mx));
}