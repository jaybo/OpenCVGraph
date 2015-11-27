#pragma once

#ifndef INCLUDE_CARTOON_HPP
#define INCLUDE_CARTOON_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;
using namespace cuda;

namespace openCVGraph
{
    // Cartoon filter
    class Cartoon : public Filter
    {
    public:

        Cartoon::Cartoon(std::string name, GraphData& graphData,
            int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool Cartoon::init(GraphData graphData) {
            bool fOK = Filter::init(graphData);
            graphData.m_NeedCV_8UC3 = true;
            return fOK;
        }

        ProcessResult Cartoon::process(GraphData& graphData)
        {
            if (graphData.m_UseCuda) {
                Mat mDebug;

                GpuMat gray;
                cuda::cvtColor(graphData.m_imOutGpu8UC3, gray, CV_BGR2GRAY);

                GpuMat tmp;
                Mat tmp3;

                gray.download(tmp3);

                GpuMat cannyOut8U;
                auto canny = cuda::createCannyEdgeDetector(50, 200);
                canny->detect(gray, cannyOut8U);
                cuda::bitwise_not(cannyOut8U, cannyOut8U);
                


                graphData.m_imOutGpu8UC3.copyTo(tmp);
                int repetitions = 7;  // Repetitions for strong cartoon effect. 
                for (int i = 0; i < repetitions; i++) {
                    int ksize = 9;     // Filter size. Has a large effect on speed.  
                    float sigmaColor = 9;    // Filter color strength.  
                    float sigmaSpace = 7;    // Spatial strength. Affects speed.  
                    cuda::bilateralFilter(graphData.m_imOutGpu8UC3, tmp, ksize, sigmaColor, sigmaSpace);
                    cuda::bilateralFilter(tmp, graphData.m_imOutGpu8UC3, ksize, sigmaColor, sigmaSpace);
                }
                //mask.download(mDebug);
                cuda::cvtColor(cannyOut8U, tmp, CV_GRAY2RGB);
                //mask.convertTo(tmp2, CV_8UC3);
                tmp.download(tmp3);
                cuda::bitwise_and(graphData.m_imOutGpu8UC3, tmp, graphData.m_imOutGpu8UC3);
                graphData.m_imOutGpu8UC3.download(graphData.m_imOut8UC3);
                graphData.m_imOut8UC3.copyTo(m_imView);

            }
            else {
                if (false) {
                    cv::stylization(graphData.m_imOut8UC3,
                        graphData.m_imOut8UC3,
                        20.f, 0.2f);
                }

                if (false) {
                    cv::edgePreservingFilter(graphData.m_imOut8UC3,
                        graphData.m_imOut8UC3
                                              );
                }
                if (false) {
                    cv::pencilSketch(graphData.m_imOut8UC3,
                        graphData.m_imOut8UC1,
                        graphData.m_imOut8UC3,
                        60,
                        0.07f,
                        0.02f
                        );
                }
                if (true) {
                    Mat gray;
                    cv::cvtColor(graphData.m_imOut8UC3, gray, CV_BGR2GRAY);
                    const int MEDIAN_BLUR_FILTER_SIZE = 7;
                    medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE);
                    Mat edges;
                    const int LAPLACIAN_FILTER_SIZE = 5;
                    Laplacian(gray, edges, CV_8U, LAPLACIAN_FILTER_SIZE);

                    Mat mask;
                    const int EDGES_THRESHOLD = 80;
                    cv::threshold(edges, mask, EDGES_THRESHOLD, 255, THRESH_BINARY_INV);

                    Mat tmp;
                    graphData.m_imOut8UC3.copyTo(tmp);
                    int repetitions = 1;  // Repetitions for strong cartoon effect. 
                    for (int i = 0; i < repetitions; i++) {
                        int ksize = 9;     // Filter size. Has a large effect on speed.  
                        double sigmaColor = 5;    // Filter color strength.  
                        double sigmaSpace = 3;    // Spatial strength. Affects speed.  
                        cv::bilateralFilter(graphData.m_imOut8UC3, tmp, ksize, sigmaColor, sigmaSpace);
                        cv::bilateralFilter(tmp, graphData.m_imOut8UC3, ksize, sigmaColor, sigmaSpace);
                    }
                    tmp.setTo(0);
                    cv::bitwise_and(graphData.m_imOut8UC3, graphData.m_imOut8UC3, tmp, mask);
                    tmp.copyTo(graphData.m_imOut8UC3);
                }
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Cartoon::processView(GraphData& graphData)
        {
            if (m_showView) {
                graphData.m_imOut8UC3.copyTo(m_imView);
                Filter::processView(graphData);
            }
        }


    private:
        cv::cuda::GpuMat CartoonOut8U;
    };
}
#endif