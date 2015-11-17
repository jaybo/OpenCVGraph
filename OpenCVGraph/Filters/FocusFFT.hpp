#pragma once

#ifndef INCLUDE_FOCUSFFT_HPP
#define INCLUDE_FOCUSFFT_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // FocusFFT filter
    class FocusFFT : public Filter
    {
    public:

        FocusFFT::FocusFFT(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool FocusFFT::process(GraphData& graphData)
        {
            if (graphData.m_UseCuda) {

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