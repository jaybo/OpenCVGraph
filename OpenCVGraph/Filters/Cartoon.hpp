#pragma once

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
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool Cartoon::init(GraphData& graphData) override
        {
            bool fOK = Filter::init(graphData);
            
            if (m_showView) {
                if (m_showViewControls) {
                    createTrackbar("Algo", m_CombinedName, &m_Algorithm, 3, NULL, this);
                    createTrackbar("r", m_CombinedName, &m_r, 1000, NULL, this);
                    createTrackbar("s", m_CombinedName, &m_s, 1000, NULL, this);
                }
            }
            return fOK;
        }

        ProcessResult Cartoon::process(GraphData& graphData) override
        {
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1);
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC3);


            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                cuda::GpuMat src;

                switch (m_StreamIn) {
                case StreamIn::CaptureProcessed:
                case StreamIn::CaptureRaw:
                    src = graphData.m_CommonData->m_imCapGpu8UC3;
                    break;
                case StreamIn::Out:
                    src = graphData.m_imOutGpu8UC3;
                    break;
                }

                GpuMat gray;
                cuda::cvtColor(src, gray, CV_BGR2GRAY);

                GpuMat tmp;
                GpuMat cannyOut8U;

                auto canny = cuda::createCannyEdgeDetector(m_r / (1000/255.0), m_s / (1000/255.0));
                canny->detect(gray, cannyOut8U);
                cuda::bitwise_not(cannyOut8U, cannyOut8U);
                
                src.copyTo(tmp);
                int repetitions = 13;  // Repetitions for strong cartoon effect. 
                for (int i = 0; i < repetitions; i++) {
                    int ksize = 9;     // Filter size. Has a large effect on speed.  
                    float sigmaColor = 11;    // Filter color strength.  
                    float sigmaSpace = 9;    // Spatial strength. Affects speed.  
                    cuda::bilateralFilter(tmp, tmp, ksize, sigmaColor, sigmaSpace);
                    cuda::bilateralFilter(tmp, tmp, ksize, sigmaColor, sigmaSpace);
                }
                cuda::cvtColor(cannyOut8U, tmp, CV_GRAY2RGB);
                cuda::bitwise_and(src, tmp, graphData.m_imOutGpu8UC3);
                graphData.m_imOutGpu8UC3.download(graphData.m_imOut8UC3);
                graphData.m_imOut8UC3.copyTo(m_imView);
#endif
            }
            else {
                Mat src, tmp;

                switch (m_StreamIn) {
                case StreamIn::CaptureProcessed:
                case StreamIn::CaptureRaw:
                    src = graphData.m_CommonData->m_imCap8UC3;
                    break;
                case StreamIn::Out:
                    src = graphData.m_imOut8UC3;
                    break;
                }

                switch (m_Algorithm)
                {
                case 0:
                    cv::stylization(src,
                        graphData.m_imOut8UC3,
                        (float)(m_r / 5.0f), (float)(m_s / 1000.0f));
                    break;
                case 1:
                    cv::edgePreservingFilter(src,
                        graphData.m_imOut8UC3,
                        1, 
                        (float) (m_r / 5.0f), (float) (m_s / 1000.0f));
                    break;
                case 2:
                    cv::pencilSketch(src,
                        graphData.m_imOut8UC1,
                        graphData.m_imOut8UC3,
                        (float)(m_r / 5.0f), (float)(m_s / 1000.0f));
                    break;
                case 3:
                    Mat gray;
                    cv::cvtColor(src, gray, CV_BGR2GRAY);
                    const int MEDIAN_BLUR_FILTER_SIZE = 7;
                    medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE);
                    Mat edges;
                    const int LAPLACIAN_FILTER_SIZE = 5;
                    Laplacian(gray, edges, CV_8U, LAPLACIAN_FILTER_SIZE);

                    Mat mask;
                    const int EDGES_THRESHOLD = 80;
                    cv::threshold(edges, mask, EDGES_THRESHOLD, 255, THRESH_BINARY_INV);

                    Mat tmp, tmp1;
                   src.copyTo(tmp);
                    int repetitions = 1;  // Repetitions for strong cartoon effect. 
                    for (int i = 0; i < repetitions; i++) {
                        int ksize = 9;     // Filter size. Has a large effect on speed.  
                        double sigmaColor = 5;    // Filter color strength.  
                        double sigmaSpace = 3;    // Spatial strength. Affects speed.  
                        cv::bilateralFilter(tmp, tmp1, ksize, sigmaColor, sigmaSpace);
                        cv::bilateralFilter(tmp1, tmp, ksize, sigmaColor, sigmaSpace);
                    }
                    tmp.copyTo(graphData.m_imOut8UC3);
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
