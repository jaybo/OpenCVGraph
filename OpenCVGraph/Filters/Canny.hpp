#pragma once

#ifndef INCLUDE_OCVG_CANNY
#define INCLUDE_OCVG_CANNY

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

        bool Canny::init(GraphData graphData) {
            bool fOK = Filter::init(graphData);
            graphData.m_NeedCV_8UC1 = true;
            return fOK;
        }

        ProcessResult Canny::process(GraphData& graphData)
        {
            if (graphData.m_UseCuda) {
                auto canny = cuda::createCannyEdgeDetector(100, 200);
                canny->detect(graphData.m_imCapGpu8UC1, cannyOut8U);
                cannyOut8U.download(graphData.m_imOut8UC1);
            }
            else {
                cv::Canny(graphData.m_imCap8UC1, graphData.m_imOut8UC1, 100, 200);
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Canny::processView(GraphData& graphData)
        {
            if (m_showView) {
                graphData.m_imOut8UC1.copyTo(m_imView);
                Filter::processView(graphData);
            }
        }

    private:
        cv::cuda::GpuMat cannyOut8U;
    };
}
#endif