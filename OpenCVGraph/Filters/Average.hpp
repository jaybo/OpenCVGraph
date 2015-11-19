#pragma once

#ifndef INCLUDE_AVERAGE_HPP
#define INCLUDE_AVERAGE_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Creates an average frame of N frames.

    class Average : public Filter
    {
    public:

        static void Average::SliderCallback(int pos, void * userData) {
            Average* filter = (Average *)userData;
            if (pos == 0) {
                pos = 1;
            }
            filter->FramesToAverage(pos);
        }

        Average::Average(std::string name, GraphData& graphData,
            int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {

        }

        bool Average::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            if (m_showView) {
                // To write on the overlay, you must allocate it.
                // This indicates to the renderer the need to merge it with the final output image.
                m_imViewOverlay = Mat(m_width, m_height, CV_8U);

                if (m_showSlider) {
                    createTrackbar("Average", m_CombinedName, &m_FramesToAverage, 10, SliderCallback, this);
                }
            }

            return true;
        }

        bool Average::process(GraphData& graphData) override
        {
            if (graphData.m_UseCuda) {
                if (m_imGpuAverage.empty()) {
                    m_imGpuAverage = cv::cuda::GpuMat(graphData.m_imCaptureGpu32F.size(), CV_32F);
                    m_imGpuAverage.setTo(Scalar(0));
                }

                cuda::add(m_imGpuAverage, graphData.m_imCaptureGpu32F, m_imGpuAverage);

                if (graphData.m_FrameNumber % m_FramesToAverage == 0) {
                    cuda::divide(m_imGpuAverage, Scalar(m_FramesToAverage), m_imGpuAverage);
                    m_imGpuAverage.setTo(Scalar(0));
                }
            }
            else {
                // todo
            }
            return true;  // if you return false, the graph stops
        }

        void Average::UpdateView(GraphData & graphData) override
        {
            if (m_showView) {
                if (graphData.m_FrameNumber % m_FramesToAverage == 0) {
                    m_imGpuAverage.convertTo(graphData.m_imResultGpu8U, CV_8U, 1.0 / 256);
                    graphData.m_imResultGpu8U.download(m_imView);
                    Filter::UpdateView(graphData);
                }
            }
        }

        void Average::FramesToAverage(int n) {
            m_FramesToAverage = n;
        }

        void  Average::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "frames_to_average" << m_FramesToAverage;
            fs << "show_slider" << m_showSlider;
        }

        void  Average::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["frames_to_average"] >> m_FramesToAverage;
            fs["show_slider"] >> m_showSlider;
        }

    private:
        cv::cuda::GpuMat m_imGpuAverage;
        int m_FramesToAverage = 3;
        bool m_showSlider = true;
        bool m_RunningAverage = true;

    };
}
#endif