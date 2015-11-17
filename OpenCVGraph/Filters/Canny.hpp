#pragma once

#ifndef INCLUDE_CANNY
#define INCLUDE_CANNY

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Canny filter
    class Canny : public Filter
    {
    public:

        Canny::Canny(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool Canny::process(GraphData& graphData)
        {
            auto canny = cuda::createCannyEdgeDetector(100, 200);
            canny->detect(graphData.m_imCaptureGpu8U, cannyOut8U);
            cannyOut8U.download(graphData.m_imResult);

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