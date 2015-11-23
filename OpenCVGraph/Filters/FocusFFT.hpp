#pragma once

#ifndef INCLUDE_FOCUSFFT_HPP
#define INCLUDE_FOCUSFFT_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Scores an image using sobel filter.  Cannot score astimatism.

    class FocusFFT : public Filter
    {
    public:

        static void FocusFFT::SliderCallback(int pos, void * userData) {
            FocusFFT* filter = (FocusFFT *)userData;
            if (!(pos & 1)) {
                pos++;      // kernels must be odd
            }
            filter->KernelSize(pos);
        }

        FocusFFT::FocusFFT(std::string name, GraphData& graphData,
            int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
            // To write on the overlay, you must allocate it.
            // This indicates to the renderer the need to merge it with the final output image.
            m_imViewTextOverlay = Mat(height, width, CV_8U);
        }

        bool FocusFFT::init(GraphData& graphData) override
        {
            Filter::init(graphData);
            if (m_showView) {
                if (m_showSlider) {
                    createTrackbar("Kernel", m_CombinedName, &m_kSize, 7, SliderCallback, this);
                }
            }

            return true;
        }

        ProcessResult FocusFFT::process(GraphData& graphData) override
        {
            if (graphData.m_UseCuda) {
                Scalar s;
                graphData.m_imOutGpu16UC1 = graphData.m_imCapGpu16UC1;
                auto nPoints = graphData.m_imCapGpu16UC1.size().area();

                // X
                m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 1, 0, m_kSize);
                m_cudaFilter->apply(graphData.m_imCapGpu16UC1, graphData.m_imOutGpu16UC1);
                s = cv::cuda::sum(graphData.m_imOutGpu16UC1);
                meanX = s[0] / nPoints;

                // Y
                m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 0, 1, m_kSize);
                m_cudaFilter->apply(graphData.m_imCapGpu16UC1, graphData.m_imOutGpu16UC1);
                s = cv::cuda::sum(graphData.m_imOutGpu16UC1);
                meanY = s[0] / nPoints;

                meanXY = (meanX + meanY) / 2;
            }
            else {
                cv::Sobel(graphData.m_imCapture, m_imSx,
                    graphData.m_imCapture.depth(),
                    1, 0,
                    m_kSize);
                cv::Sobel(graphData.m_imCapture, m_imSy,
                    graphData.m_imCapture.depth(),
                    0, 1,
                    m_kSize);

                meanX = cv::mean(m_imSx)[0];
                meanY = cv::mean(m_imSy)[0];
                meanXY = (meanX + meanY) / 2;
            }
            if (m_showView) {
                graphData.m_imCap8UC1.copyTo(m_imView);
                DrawOverlay(graphData);
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void FocusFFT::DrawOverlay(GraphData graphData)
        {
            m_imViewTextOverlay = 0;
            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "  meanXY    X      Y";
            DrawShadowTextMono(m_imViewTextOverlay, str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << setw(7) << (int)meanXY << setw(7) << (int)meanX << setw(7) << (int)meanY;
            DrawShadowTextMono(m_imViewTextOverlay, str.str(), Point(posLeft, 100), scale);
        }

        void FocusFFT::KernelSize(int kernelSize) {
            m_kSize = kernelSize;
        }

        void  FocusFFT::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "kernel_size" << m_kSize;
            fs << "show_slider" << m_showSlider;
        }

        void  FocusFFT::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["kernel_size"] >> m_kSize;
            fs["show_slider"] >> m_showSlider;
        }

    private:
        Mat m_imSx;
        Mat m_imSy;
        double meanX, meanY, meanXY;
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
        int m_kSize = 3;
        bool m_showSlider = true;

    };
}
#endif