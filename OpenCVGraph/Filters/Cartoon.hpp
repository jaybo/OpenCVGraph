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
    // If CUDA is disabled, can show 3 different cartoon effects.  Use slider to select algo and parameters.

    class Cartoon : public Filter
    {
    public:
        Cartoon::Cartoon(std::string name, GraphData& graphData,
            int sourceFormat = CV_8UC3,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }

        bool Cartoon::init(GraphData& graphData) override
        {
            bool fOK = Filter::init(graphData);
            
            if (m_Enabled) {
                graphData.m_NeedCV_8UC1 = true;
                graphData.m_NeedCV_8UC3 = true;

                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("Algo", m_CombinedName, &m_Algorithm, 4, NULL, this);
                        createTrackbar("r", m_CombinedName, &m_r, 1000, NULL, this);
                        createTrackbar("s", m_CombinedName, &m_s, 1000, NULL, this);
                    }
                }
            }
            return fOK;
        }

        ProcessResult Cartoon::process(GraphData& graphData) override
        {
            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                GpuMat gray;
                cuda::cvtColor(graphData.m_imOutGpu8UC3, gray, CV_BGR2GRAY);

                GpuMat tmp;
                GpuMat cannyOut8U;

                auto canny = cuda::createCannyEdgeDetector(m_r / (1000/255.0), m_s / (1000/255.0));
                canny->detect(gray, cannyOut8U);
                cuda::bitwise_not(cannyOut8U, cannyOut8U);
                
                graphData.m_imOutGpu8UC3.copyTo(tmp);
                int repetitions = 13;  // Repetitions for strong cartoon effect. 
                for (int i = 0; i < repetitions; i++) {
                    int ksize = 9;     // Filter size. Has a large effect on speed.  
                    float sigmaColor = 11;    // Filter color strength.  
                    float sigmaSpace = 9;    // Spatial strength. Affects speed.  
                    cuda::bilateralFilter(graphData.m_imOutGpu8UC3, tmp, ksize, sigmaColor, sigmaSpace);
                    cuda::bilateralFilter(tmp, graphData.m_imOutGpu8UC3, ksize, sigmaColor, sigmaSpace);
                }
                cuda::cvtColor(cannyOut8U, tmp, CV_GRAY2RGB);
                cuda::bitwise_and(graphData.m_imOutGpu8UC3, tmp, graphData.m_imOutGpu8UC3);
                graphData.m_imOutGpu8UC3.download(graphData.m_imOut8UC3);
                graphData.m_imOut8UC3.copyTo(m_imView);
#endif
            }
            else {
                switch (m_Algorithm)
                {
                case 0:
                    cv::stylization(graphData.m_imOut8UC3,
                        graphData.m_imOut8UC3,
                        (float)(m_r / 5.0f), (float)(m_s / 1000.0f));
                    break;
                case 1:
                    cv::edgePreservingFilter(graphData.m_imOut8UC3,
                        graphData.m_imOut8UC3,
                        1, 
                        (float) (m_r / 5.0f), (float) (m_s / 1000.0f));
                    break;
                case 2:
                    cv::pencilSketch(graphData.m_imOut8UC3,
                        graphData.m_imOut8UC1,
                        graphData.m_imOut8UC3,
                        (float)(m_r / 5.0f), (float)(m_s / 1000.0f));
                    break;
                case 3:
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
                    break;
                }
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Cartoon::processView(GraphData& graphData) override
        {
            if (m_showView) {
                if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                    graphData.m_imOutGpu8UC3.download(m_imView);
#endif
                }
                else
                {
                    m_imView = graphData.m_imOut8UC3;
                }
                Filter::processView(graphData);
            }
        }

        void  Cartoon::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "algorithm" << m_Algorithm;
            fs << "r" << m_r;
            fs << "s" << m_s;
        }

        void  Cartoon::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["algorithm"] >> m_Algorithm;
            fs["r"] >> m_r;
            fs["s"] >> m_s;
        }

    private:
        int m_Algorithm = 0;
        int m_r = 100;    // range 0 - 200 for most filters
        int m_s = 500;    // range 0 - 1 for most filters
#ifdef WITH_CUDA
        cv::cuda::GpuMat CartoonOut8U;
#endif

    };
}
#endif