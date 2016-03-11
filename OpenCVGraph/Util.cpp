#pragma once 

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

void createDir(std::string& dir) {
#if defined _MSC_VER
    _mkdir(dir.c_str());
#elif defined __GNUC__
    mkdir(dir.c_str(), 0777);
#endif
}

std::string GetFileExtension(const std::string& FileName)
{
    if (FileName.find_last_of(".") != std::string::npos)
        return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
}

int getU16Pix(const cv::Mat& img, cv::Point pt)
{
    cv::Mat patch;
    cv::remap(img, patch, cv::Mat(1, 1, CV_16UC1, &pt), cv::noArray(),
        cv::INTER_NEAREST, cv::BORDER_CONSTANT, Scalar(128, 128, 128));
    return patch.at<int>(0, 0);
    return 0;
}

// Returns a histogram image for 8 or 16BPP input image
// The returned histogram image is CV_8UC1

Mat createGrayHistogram(Mat& img, int bins, int width, int height)
{
    int nc = img.channels();
    int depth = img.depth();
    bool is16bpp = (depth == CV_16U);

    if (!img.data)
    {
        auto m = Mat(width, height, CV_8UC1);
        m.setTo(0);
        return m;
    }

    Mat img2(img);
    if (nc == 3) {
        // using WebCam, fake it
        cv::cvtColor(img, img2, CV_RGB2GRAY);
    }

    /// Establish the number of bins
    int histSize = bins;

    float range[] = { 0, (float)(is16bpp ? 65535 : 255) };
    const float* histRange = { range };

    bool uniform = true;
    bool accumulate = false;

    Mat hist;

    /// Compute the histograms:
    cv::calcHist(&img2, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

    // Draw the histogram
    int hist_w = width; int hist_h = height;
    int bin_w = cvRound((double)hist_w / histSize);

    Mat histImage(hist_h, hist_w, CV_8UC1);
    histImage.setTo(0);

    /// Normalize the result to [ 0, histImage.rows ]
    cv::normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

    /// Draw for each channel
    for (int i = 1; i < histSize; i++)
    {
        line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
            Point(bin_w*(i), hist_h - cvRound(hist.at<float>(i))),
            Scalar(255, 255, 255), 2, 8, 0);
    }

    return histImage;
}

