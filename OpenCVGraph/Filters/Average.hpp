#pragma once

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Creates an average frame of N frames.
    // Currently source and result must be CV_32FC1

    class Average : public Filter
    {
    public:
        Average::Average(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool Average::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            if (m_showView) {
                if (m_showViewControls) {
                    createTrackbar("Average", m_CombinedName, &m_FramesToAverage, 16, SliderCallback, this);
                }
            }
            return true;
        }

        ProcessResult Average::process(GraphData& graphData) override
        {
            ProcessResult result = ProcessResult::Continue;  // Averaging filters return this to skip the rest of the loop
            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                cuda::GpuMat src;
                    
                switch (m_StreamIn) {
                case StreamIn::CaptureProcessed:
                    // bugbug todo, this ain't right!
                    graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_32F);
                    src = graphData.m_CommonData->m_imCapGpu32FC1;
                    break;
                case StreamIn::CaptureRaw:
                    graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_32F);
                    src = graphData.m_CommonData->m_imCapGpu32FC1;
                    break;
                case StreamIn::Out:
                    src = graphData.m_imOutGpu32FC1;
                    break;
                }
                if (m_imGpuAverage32F.empty()) {
                    m_imGpuAverage32F = cv::cuda::GpuMat(src.size(), CV_32F);
                    m_imGpuAverage32F.setTo(Scalar(0));
                }

                cuda::add(m_imGpuAverage32F, src, m_imGpuAverage32F);

                if ((m_FramesAveraged > 0) && (m_FramesAveraged % m_FramesToAverage == 0)) {
                    cuda::divide(m_imGpuAverage32F, Scalar(m_FramesToAverage), m_imGpuAverage32F);
                    m_imGpuAverage32F.copyTo(graphData.m_imOutGpu32FC1);
                    m_imGpuAverage32F.convertTo(graphData.m_imOutGpu16UC1, CV_16U);
                    if (m_showView) {
                        m_imGpuAverage32F.convertTo(graphData.m_imOutGpu8UC1, CV_8U, 1.0 / 256);
                    }
                    m_imGpuAverage32F.setTo(Scalar(0));
                    result = ProcessResult::OK;  // Let this sample continue onto the graph
                }
#else
                assert(false);
#endif
            }
            else {
                Mat src;

                switch (m_StreamIn) {
                case StreamIn::CaptureProcessed:
                    // bugbug todo, this ain't right!
                    graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_32F);
                    src = graphData.m_CommonData->m_imCap32FC1;
                    break;
                case StreamIn::CaptureRaw:
                    graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_32F);
                    src = graphData.m_CommonData->m_imCap32FC1;
                    break;
                case StreamIn::Out:
                    src = graphData.m_imOut32FC1;
                    break;
                }

                //todo
                if (m_imAverage32F.empty()) {
                    m_imAverage32F = cv::Mat(src.size(), CV_32F);
                    m_imAverage32F.setTo(Scalar(0));
                }

                cv::add(m_imAverage32F, src, m_imAverage32F);

                if ((m_FramesAveraged > 0) && (m_FramesAveraged % m_FramesToAverage == 0)) {
                    if (m_FramesToAverage > 1) {
                        cv::divide(m_imAverage32F, Scalar(m_FramesToAverage), m_imAverage32F);
                    }
                    m_imAverage32F.copyTo(graphData.m_imOut32FC1);
                    m_imAverage32F.convertTo(graphData.m_imOut16UC1, CV_16U);
                    m_imAverage32F.convertTo(graphData.m_imOut8UC1, CV_8U, 1.0 / 256);
                    m_imAverage32F.setTo(Scalar(0));

                    result = ProcessResult::OK;  // Let this sample continue onto the graph
                }

            }
            m_FramesAveraged++;

            return result;  // if you return false, the graph stops
        }

        void Average::processView(GraphData & graphData) override
        {
            if (m_showView) {
                if ((m_FramesAveraged > 0) && (m_FramesAveraged % m_FramesToAverage == 0)) {
                    if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                        graphData.m_imOutGpu8UC1.download(m_imView);
#endif
                    }
                    else {
                        m_imView = graphData.m_imOut8UC1;
                    }
                    Filter::processView(graphData);

                }
            }
        }

        void Average::FramesToAverage(int n) {
            m_FramesToAverage = n;
            m_FramesAveraged = 0;
        }

        void  Average::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "frames_to_average" << m_FramesToAverage;
        }

        void  Average::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["frames_to_average"] >> m_FramesToAverage;
        }

    private:
#ifdef WITH_CUDA
        cv::cuda::GpuMat m_imGpuAverage32F;
#endif
        Mat m_imAverage32F;
        int m_FramesToAverage = 4;
        int m_FramesAveraged = 0;

        static void Average::SliderCallback(int pos, void * userData) {
            Average* filter = (Average *)userData;
            if (pos == 0) {
                pos = 1;
            }
            filter->FramesToAverage(pos);
        }
    };
}
