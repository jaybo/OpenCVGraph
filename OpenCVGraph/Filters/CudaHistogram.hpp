#pragma once

#ifndef INCLUDE_CUDAHISTOGRAM
#define INCLUDE_CUDAHISTOGRAM

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Simplest possible filter
    class CudaHistogram : public Filter
    {

    public:

        CudaHistogram::CudaHistogram(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, showView, width, height)
        {
        }

        bool CudaHistogram::process(GraphData& gd)
        {
            int nc = gd.m_imCapture.channels();    // number of channels
            int depth = gd.m_imCapture.depth();
            bool is16bpp = (depth == CV_16U);

            /// Establish the number of bins
            int histSize = 256;

            /// Set the range
            float range[] = { 0, (float)(is16bpp ? 65535 : 255) };
            const float* histRange = { range };

            bool uniform = true; bool accumulate = false;

            Mat hist;

            // capture to gpu
            m_imGpu.upload(gd.m_imCapture);

            /// Compute the histograms:
            // cuda::histRange(m_imGpu, m_hist, 255);

            // cuda::CannyEdgeDetector::detect(m_imGpu, m_hist);

            cv::cuda::lshift(m_imGpu, 4, m_imGpu);
            

            if (m_showView) {
                gd.m_imCapture.copyTo(m_imView);
            }
            return true;  // if you return false, the graph stops
        }

    private:
        cuda::GpuMat m_imGpu;
        cuda::GpuMat m_hist;
    };
}
#endif