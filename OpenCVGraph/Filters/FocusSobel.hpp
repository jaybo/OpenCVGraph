#pragma once

#ifndef INCLUDE_FOCUSSOBEL_HPP
#define INCLUDE_FOCUSSOBEL_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Scores an image using sobel filter.  Cannot score astimatism.

    class FocusSobel : public Filter
    {
    public:

        FocusSobel::FocusSobel(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
            // To write on the overlay, you must allocate it.
            // This indicates to the renderer the need to merge it with the final output image.
            m_imViewOverlay = Mat(height, width, CV_8U);
        }

        static void FocusSobel::SliderCallback(int pos, void * userData) {
            FocusSobel* filter = (FocusSobel *)userData;
            filter->KernelSize(pos);
        }

        bool FocusSobel::init(GraphData graphData) {
            Filter::init(graphData);
            if (m_showView) {
                if (m_showSlider) {
                    createTrackbar("Kernel", m_CombinedName, &m_kSize, 20, SliderCallback, this);
                }
            }
        }

        bool FocusSobel::process(GraphData& graphData)
        {
            if (graphData.m_UseCuda) {

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
                graphData.m_imCapture8U.copyTo(m_imView);
                DrawOverlay(graphData);
            }
            return true;  // if you return false, the graph stops
        }

        void FocusSobel::DrawOverlay(GraphData graphData) {
            m_imViewOverlay = 0;
            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "   mean     X      Y";
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << setw(7) << (int) meanXY << setw(7) << (int) meanX << setw(7) << (int) meanY;
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 100), scale);
        }

        void FocusSobel::KernelSize(int kernelSize) {
            m_kSize = kernelSize;
        }

        void  FocusSobel::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "kernel_size" << m_kSize;
            fs << "show_slider" << m_showSlider;
        }

        void  FocusSobel::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["kernel_size"] >> m_kSize;
            fs["show_slider"] >> m_showSlider;
        }

    private:
        Mat m_imSx;
        Mat m_imSy;
        double meanX, meanY, meanXY;
        //cv::cuda::GpuMat foo8U;
        int m_kSize = 3;
        bool m_showSlider = true;

    };
}
#endif