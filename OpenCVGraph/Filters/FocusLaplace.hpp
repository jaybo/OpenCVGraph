#pragma once

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
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool FocusLaplace::init(GraphData& graphData) override
        {
            Filter::init(graphData);

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
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, false);

            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                m_imGpuLaplace = graphData.m_CommonData->m_imCaptureGpu8UC1.clone();
                auto nPoints = graphData.m_CommonData->m_imCaptureGpu8UC1.size().area();

                m_cudaFilter = cv::cuda::createLaplacianFilter(graphData.m_CommonData->m_imCaptureGpu8UC1.type(), m_imGpuLaplace.type(),
                    (m_kSize == 1 || m_kSize == 3) ? m_kSize : 3);  // only 1 or 3 in cuda
                m_cudaFilter->apply(graphData.m_CommonData->m_imCaptureGpu8UC1, m_imGpuLaplace);

                Scalar mean, std;
                m_imGpuLaplace.download(graphData.m_imOut);
                cv::cuda::meanStdDev(m_imGpuLaplace, mean, std);
                m_var = std[0] * std[0];
#else
                assert(0);
#endif
            }
            else {
                m_imLaplace = graphData.m_CommonData->m_imCapture.clone();
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
        //GpuMat m_imGpuTemp;

        double m_var;
#ifdef WITH_CUDA
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
#endif
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
