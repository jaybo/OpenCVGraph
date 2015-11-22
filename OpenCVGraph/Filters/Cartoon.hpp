#pragma once

#ifndef INCLUDE_CARTOON_HPP
#define INCLUDE_CARTOON_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

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
            }
            else {
                if (true) {
                    cv::stylization(graphData.m_imResult8UC3,
                        graphData.m_imResult8UC3,
                        20.f, 0.2f);
                }

                if (false) {
                    cv::edgePreservingFilter(graphData.m_imResult8UC3,
                        graphData.m_imResult8UC3
                                              );
                }
                if (false) {
                    cv::pencilSketch(graphData.m_imResult8UC3,
                        graphData.m_imResult8U,
                        graphData.m_imResult8UC3,
                        60,
                        0.07f,
                        0.02f
                        );
                }
                if (false) {
                    Mat gray;
                    cvtColor(graphData.m_imResult8UC3, gray, CV_BGR2GRAY);
                    const int MEDIAN_BLUR_FILTER_SIZE = 7;
                    medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE);
                    Mat edges;
                    const int LAPLACIAN_FILTER_SIZE = 5;
                    Laplacian(gray, edges, CV_8U, LAPLACIAN_FILTER_SIZE);

                    Mat mask;
                    const int EDGES_THRESHOLD = 80;
                    threshold(edges, mask, EDGES_THRESHOLD, 255, THRESH_BINARY_INV);

                    Mat tmp;
                    graphData.m_imResult8UC3.copyTo(tmp);
                    int repetitions = 7;  // Repetitions for strong cartoon effect. 
                    for (int i = 0; i < repetitions; i++) {
                        int ksize = 9;     // Filter size. Has a large effect on speed.  
                        double sigmaColor = 9;    // Filter color strength.  
                        double sigmaSpace = 7;    // Spatial strength. Affects speed.  
                        bilateralFilter(graphData.m_imResult8UC3, tmp, ksize, sigmaColor, sigmaSpace);
                        bilateralFilter(tmp, graphData.m_imResult8UC3, ksize, sigmaColor, sigmaSpace);
                    }
                }
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Cartoon::processView(GraphData& graphData)
        {
            if (m_showView) {
                graphData.m_imResult8UC3.copyTo(m_imView);
                Filter::processView(graphData);
            }
        }


    private:
        cv::cuda::GpuMat CartoonOut8U;
    };
}
#endif