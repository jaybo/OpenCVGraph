#pragma once

#ifndef INCLUDE_FOCUS_HPP
#define INCLUDE_FOCUS_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Focus filter
    class Focus : public Filter
    {
    public:

        Focus::Focus(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool Focus::process(GraphData& graphData)
        {
            if (graphData.m_UseCuda) {
                auto canny = cuda::createCannyEdgeDetector(100, 200);
                canny->detect(graphData.m_imCaptureGpu8U, cannyOut8U);
                cannyOut8U.download(graphData.m_imResult);
            }
            else {
               
            }
            if (m_showView) {
                graphData.m_imResult.copyTo(m_imView);
            }
            return true;  // if you return false, the graph stops
        }
    private:
        cv::cuda::GpuMat cannyOut8U;
    };
}
#endif