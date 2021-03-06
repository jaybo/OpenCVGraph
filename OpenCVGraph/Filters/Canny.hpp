#pragma once

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
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool Canny::init(GraphData& graphData)  override
        {
            bool fOK = Filter::init(graphData);

            if (m_showView) {
                if (m_showViewControls) {
                    createTrackbar("Th 1", m_CombinedName, &m_Threshold1, 255, Slider1Callback, this);
                    createTrackbar("Th 2", m_CombinedName, &m_Threshold2, 255, Slider2Callback, this);
                }
            }
            return fOK;
        }

        ProcessResult Canny::process(GraphData& graphData) override
        {
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, false);

            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                auto canny = cuda::createCannyEdgeDetector(m_Threshold1, m_Threshold2);
                canny->detect(graphData.m_CommonData->m_imCaptureGpu8UC1, cannyOut8U);
                cannyOut8U.download(graphData.m_imOut);
#else
                assert(false);
#endif
            }
            else {
                cv::Canny(graphData.m_CommonData->m_imCapture8UC1, graphData.m_imOut, m_Threshold1, m_Threshold2);
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Canny::processView(GraphData& graphData) override
        {
            if (m_showView) {
                m_imView = graphData.m_imOut;
                Filter::processView(graphData);
            }
        }

        void  Canny::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "threshold1" << m_Threshold1;
            fs << "threshold2" << m_Threshold2;
        }

        void  Canny::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["threshold1"] >> m_Threshold1;
            fs["threshold2"] >> m_Threshold2;
        }

    private:

        static void Canny::Slider1Callback(int pos, void * userData) {
            Canny* filter = (Canny *)userData;
            filter->m_Threshold1 = pos;
        }

        static void Canny::Slider2Callback(int pos, void * userData) {
            Canny* filter = (Canny *)userData;
            filter->m_Threshold2 = pos;
        }

#ifdef WITH_CUDA
        cv::cuda::GpuMat cannyOut8U;
#endif
        int m_Threshold1 = 100;
        int m_Threshold2 = 200;
    };
}
