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


vector<Mat> createHistogramImages(Mat& img, int bins, int width, int height)
{
    int nc = img.channels();    // number of channels
    int depth = img.depth();
    bool is16bpp = (depth == CV_16U);

    vector<Mat> hist(nc);       // histogram arrays

                                // Initalize histogram arrays
    for (int i = 0; i < hist.size(); i++)
        hist[i] = Mat::zeros(1, bins, CV_32SC1);

    // Calculate the histogram of the image
    for (int i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            for (int k = 0; k < nc; k++)
            {
                if (is16bpp) {
                    ushort val = img.at<ushort>(i, j) / 256; 
                    hist[k].at<int>(val) += 1;
                }
                else {
                    uchar val = nc == 1 ? img.at<uchar>(i, j) : img.at<Vec3b>(i, j)[k];
                    hist[k].at<int>(val) += 1;
                }
            }
        }
    }

    // For each histogram arrays, obtain the maximum (peak) value
    // Needed to normalize the display later
    int hmax[3] = { 0,0,0 };
    for (int i = 0; i < nc; i++)
    {
        for (int j = 0; j < bins - 1; j++)
            hmax[i] = hist[i].at<int>(j) > hmax[i] ? hist[i].at<int>(j) : hmax[i];
    }

    const char* wname[3] = { "blue", "green", "red" };
    Scalar colors[3] = { Scalar(255,0,0), Scalar(0,255,0), Scalar(0,0,255) };

    vector<Mat> canvas(nc);

    // Display each histogram in a canvas
    for (int i = 0; i < nc; i++)
    {
        canvas[i] = Mat::ones(125, bins, CV_8UC3);

        for (int j = 0, rows = canvas[i].rows; j < bins - 1; j++)
        {
            line(
                canvas[i],
                Point(j, rows),
                Point(j, rows - (hist[i].at<int>(j) * rows / hmax[i])),
                nc == 1 ? Scalar(200, 200, 200) : colors[i],
                1, 8, 0
                );
        }

        imshow(nc == 1 ? "value" : wname[i], canvas[i]);
    }
    return canvas;
}

