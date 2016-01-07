#pragma once

#ifndef INCLUDE_OCVG_FOCUS_LAPLACE_HPP
#define INCLUDE_OCVG_FOCUS_LAPLACE_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Scores an image using sobel filter.  Cannot score astimatism.

    class FocusLaplace : public Filter
    {
    public:

        FocusLaplace::FocusLaplace(std::string name, GraphData& graphData,
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }

        bool FocusLaplace::init(GraphData& graphData) override
        {
            Filter::init(graphData);
            graphData.m_NeedCV_8UC1 = true;

            if (m_Enabled) {
                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("Kernel", m_CombinedName, &m_kSize, 7, SliderCallback, this);
                    }
                }
            }
            return true;
        }

        ProcessResult FocusLaplace::process(GraphData& graphData) override
        {
            if (graphData.m_UseCuda) {
                graphData.m_imOutGpu16UC1 = graphData.m_CommonData->m_imCapGpu16UC1;
                auto nPoints = graphData.m_CommonData->m_imCapGpu16UC1.size().area();

                m_cudaFilter = cv::cuda::createLaplacianFilter(graphData.m_CommonData->m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 
                    (m_kSize == 1 || m_kSize == 3) ? m_kSize : 3);  // only 1 or 3 in cuda
                m_cudaFilter->apply(graphData.m_CommonData->m_imCapGpu16UC1, m_imGpuLaplace);

                Scalar mean, std;
                m_imGpuLaplace.convertTo(m_imGpuTemp, CV_8UC1, 1/256.0f);
                m_imGpuLaplace = m_imGpuTemp;
                cv::cuda::meanStdDev(m_imGpuTemp, mean, std);
                m_var = std[0] * std[0];
            }
            else {
                cv::Laplacian(graphData.m_CommonData->m_imCapture, m_imLaplace,
                    graphData.m_CommonData->m_imCapture.depth(),
                    m_kSize,
                    1, 0
                    );
                m_imLaplace.convertTo(m_imLaplace, CV_8UC1, 1/256.0);
                cv::meanStdDev(m_imLaplace, m_imMean, m_imStdDev);
                double sd = m_imStdDev.at<double> (0,0);
                m_var = sd * sd;
            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void FocusLaplace::processView(GraphData& graphData)  override
        {
            ClearOverlayText();

            if (graphData.m_UseCuda) {
                m_imGpuLaplace.download(m_imView);
            }
            else
            {
                m_imView = m_imLaplace;
            }

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "Focus: Laplacian";
            DrawOverlayText(str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << m_var;
            DrawOverlayText(str.str(), Point(posLeft, 100), scale);

            Filter::processView(graphData);
        }

        void  FocusLaplace::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "kernel_size" << m_kSize;
        }

        void  FocusLaplace::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["kernel_size"] >> m_kSize;
        }

        void FocusLaplace::KernelSize(int kernelSize) {
            m_kSize = kernelSize;
        }

    private:
        Mat m_imLaplace;
        Mat m_imMean;
        Mat m_imStdDev;
        GpuMat m_imGpuLaplace;
        GpuMat m_imGpuTemp;

        double m_var;
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
        int m_kSize = 3;

        static void FocusLaplace::SliderCallback(int pos, void * userData) {
            FocusLaplace* filter = (FocusLaplace *)userData;
            if (!(pos & 1)) {
                pos++;      // kernels must be odd
            }
            filter->KernelSize(pos);
        }
    };
}
#endif