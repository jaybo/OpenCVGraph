#pragma once 

#include "stdafx.h"

using namespace std;

//namespace OpenCVGraph {

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

    void createDir() {
    }
//    void createDir(std::string& dir) {
//#if defined _MSC_VER
//        _mkdir(dir.c_str());
//#elif defined __GNUC__
//        mkdir(dir.c_str(), 0777);
//#endif
//    }

    void DrawShadowTextMono(cv::Mat m, string str, cv::Point p, double scale)
    {
        int offset = 3;
        cv::Point ptOffset = p;
        ptOffset.x += 2;
        ptOffset.y += 2;
        int depth = m.depth();
        double hi = 255;
        double lo = 128;
        if (depth <= 1) {   // 8bit
            hi = 255;
            lo = hi / 2;

        }
        else if (depth <= 3) { // 16bit
            hi = 65535;
            lo = hi / 2;
        }
        cv::putText(m, str, ptOffset, CV_FONT_HERSHEY_DUPLEX, scale, CV_RGB(lo, lo, lo));  // shadow
        cv::putText(m, str, p, CV_FONT_HERSHEY_DUPLEX, scale, CV_RGB(hi, hi, hi));
    }

    int getU16Pix(const cv::Mat& img, cv::Point pt)
    {
        cv::Mat patch;
        cv::remap(img, patch, cv::Mat(1, 1, CV_16UC1, &pt), cv::noArray(),
            cv::INTER_NEAREST, cv::BORDER_CONSTANT, Scalar(128, 128, 128));
        return patch.at<int>(0, 0);
        return 0;
    }

    vector<Mat> createGrayHistogram(Mat& img, int bins, int width, int height)
    {
        int nc = img.channels();    // number of channels
        int depth = img.depth();
        bool is16bpp = (depth == CV_16U);

        //if (!img.data)
        //{
        //    return NULL;
        //}


        /// Establish the number of bins
        int histSize = bins;

        /// Set the ranges ( for B,G,R) )
        float range[] = { 0, (float)(is16bpp ? 65535 : 255) };
        const float* histRange = { range };

        bool uniform = true; bool accumulate = false;

        Mat hist;

        /// Compute the histograms:
        calcHist(&img, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

        // Draw the histogram
        int hist_w = width; int hist_h = height;
        int bin_w = cvRound((double)hist_w / histSize);

        Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

        /// Normalize the result to [ 0, histImage.rows ]
        normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

        /// Draw for each channel
        for (int i = 1; i < histSize; i++)
        {
            line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
                Point(bin_w*(i), hist_h - cvRound(hist.at<float>(i))),
                Scalar(255, 0, 0), 2, 8, 0);
        }

        /// Display
        namedWindow("calcHist Demo", CV_WINDOW_AUTOSIZE);
        imshow("calcHist Demo", histImage);

        return histImage;
    }

//}