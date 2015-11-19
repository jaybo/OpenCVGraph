#pragma once

#ifndef INCLUDE_OCVG_AVERAGE
#define INCLUDE_OCVG_AVERAGE

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

            graphData.m_NeedCV_32FC1 = true;

            if (m_showView) {
                // To write on the overlay, you must allocate it.
                // This indicates to the renderer the need to merge it with the final output image.
                // m_imViewTextOverlay = Mat(m_width, m_height, CV_8U);

                if (m_showSlider) {
                    createTrackbar("Average", m_CombinedName, &m_FramesToAverage, 10, SliderCallback, this);
                }
            }

            return true;
        }

        ProcessResult Average::process(GraphData& graphData) override
        {
            ProcessResult result = ProcessResult::Continue;  // Averaging filters return this to skip the rest of the loop

            if (graphData.m_UseCuda) {
                if (m_imGpuAverage32F.empty()) {
                    m_imGpuAverage32F = cv::cuda::GpuMat(graphData.m_imResultGpu32F.size(), CV_32F);
                    m_imGpuAverage32F.setTo(Scalar(0));
                }

                cuda::add(m_imGpuAverage32F, graphData.m_imResultGpu32F, m_imGpuAverage32F);

                if ((m_FramesAveraged > 0) && (m_FramesAveraged % m_FramesToAverage == 0)) {
                    cuda::divide(m_imGpuAverage32F, Scalar(m_FramesToAverage), m_imGpuAverage32F);
                    m_imGpuAverage32F.copyTo(graphData.m_imResultGpu32F);
                    m_imGpuAverage32F.convertTo(graphData.m_imResultGpu16U, CV_16U);
                    m_imGpuAverage32F.convertTo(graphData.m_imResultGpu8U, CV_8U, 1.0/256);
                    m_imGpuAverage32F.setTo(Scalar(0));
                    result = ProcessResult::OK;  // Let this sample continue onto the graph
                }
            }
            else {
                //todo
                assert(false);

            }
            m_FramesAveraged++;

            return result;  // if you return false, the graph stops
        }

        void Average::processView(GraphData & graphData) override
        {
            if (m_showView) {
                if (graphData.m_FrameNumber % m_FramesToAverage == 0) {
                    graphData.m_imResultGpu8U.download(m_imView);
                    Filter::processView(graphData);
                }
            }
        }

        void Average::FramesToAverage(int n) {
            m_FramesToAverage = n;
            m_FramesAveraged = 0;
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
        cv::cuda::GpuMat m_imGpuAverage32F;
        int m_FramesToAverage = 3;
        int m_FramesAveraged = 0;
        bool m_showSlider = true;
        // bool m_RunningAverage = true;

    };
}
#endif