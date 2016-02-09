#pragma once

#include "..\stdafx.h"

using namespace cv;

namespace openCVGraph
{
    // Collects basic statistics about the image
    // Min, Max, Mean, and 256 element histogram

    class ImageQC : public Filter, public ITemcaQC
    {
    public:
        ImageQC(std::string name, GraphData& graphData, 
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }


        //Allocate resources if needed
        bool ImageQC::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            m_UseCuda = graphData.m_UseCuda;            // Need this for a function which doesn't pass graphData!

            return true;
        }

        // deallocate resources
        bool ImageQC::fini(GraphData& graphData) override
        {
            return true;
        }

        ProcessResult ImageQC::process(GraphData& graphData) override
        {
            bool useCorrected = (m_StreamIn == StreamIn::Corrected);

            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_16UC1, useCorrected);
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, useCorrected);

            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                cuda::GpuMat src16, src8;
                if (useCorrected) {
                    src16 = graphData.m_CommonData->m_imCorrectedGpu16UC1;
                    src8 = graphData.m_CommonData->m_imCorrectedGpu8UC1;
                }
                else {
                    src16 = graphData.m_CommonData->m_imCaptureGpu16UC1;
                    src8 = graphData.m_CommonData->m_imCaptureGpu8UC1;
                }
                cv::cuda::minMax(src16, &m_dCapMin, &m_dCapMax);
                cv::cuda::calcHist(src8, m_histogramGpu);
                
                // This version fails!!!
                // cv::cuda::histEven(graphData.m_CommonData->m_imCaptureGpu16UC1, m_histogramGpu, 256, 0, UINT16_MAX);

                auto nPoints = src16.size().area();
                m_Mean = (int) (cv::cuda::sum(src16)[0] / nPoints);
#endif
            }
            else {
                Mat src16, src8;
                if (useCorrected) {
                    src16 = graphData.m_CommonData->m_imCorrected16UC1;
                    src8 = graphData.m_CommonData->m_imCorrected8UC1;
                }
                else {
                    src16 = graphData.m_CommonData->m_imCapture16UC1;
                    src8 = graphData.m_CommonData->m_imCapture8UC1;
                }

                cv::minMaxLoc(src16, &m_dCapMin, &m_dCapMax);
                int channels[] = { 0 };
                int histSize[] = { 256 };
                float range[] = { 0, 256 };
                const float* ranges[] = { range };

                cv::calcHist(&src8, 1, channels, Mat(), m_histogram, 1, histSize, ranges, true, false);

                auto nPoints = src16.size().area();
                m_Mean = (int)(cv::sum(src16)[0] / nPoints);
            }
            return ProcessResult::OK;
        }



        void  ImageQC::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
        }

        void  ImageQC::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
        }

        // ITemcaQC
        QCInfo ImageQC::getQCInfo() {
            QCInfo info;
            if (m_UseCuda) {
#ifdef WITH_CUDA
                if (!m_histogramGpu.empty()) {
                    m_histogramGpu.download(m_histogram); 
                }
#endif
            }
            if (!m_histogram.empty()) {
                const int32_t * p = m_histogram.ptr<int32_t>(0);

                for (int i = 0; i < m_histogram.cols; i++) {
                    info.histogram[i] = p[i];
                }
            }
            else {
                for (int i = 0; i < 256; i++) {
                    info.histogram[i] = 0;
                }
            }
            info.max_value = (int) m_dCapMax;
            info.min_value = (int) m_dCapMin;
            info.mean_value = m_Mean;
            return info;
        }

    private:
        int m_Mean;
        cv::cuda::GpuMat m_histogramGpu;
        Mat m_histogram;
        double m_dCapMax, m_dCapMin;
        bool m_UseCuda;

        void ImageQC::processView(GraphData& graphData)
        {
            // bugbug test
            // getQCInfo();

            if (m_showView) {
                m_imView = graphData.m_CommonData->m_imCapture;
                DrawOverlay(graphData);
                Filter::processView(graphData);
            }
        }

        void ImageQC::DrawOverlay(GraphData& graphData) {
            ClearOverlayText();

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "         min   mean   max";
            DrawOverlayText(str.str(), Point(posLeft, 30), scale);

            str.str("");
            str << "Cap:" << std::setfill(' ') << setw(7) << (int)m_dCapMin << setw(7) << (int)m_Mean << setw(7) << (int)m_dCapMax;
            DrawOverlayText(str.str(), Point(posLeft, 70), scale);

            str.str("");
            str << graphData.m_FrameNumber;
            DrawOverlayText(str.str(), Point(posLeft, 500), scale);

            auto histSize = Size(512, 200);
            // bugbug todo, the following unnecessarily recalcs the histogram
            Mat histo = createGrayHistogram(graphData.m_CommonData->m_imCapture, 256, histSize.width, histSize.height);
            cv::cvtColor(histo, histo, CV_GRAY2RGB);

            Mat t = Mat(m_imViewTextOverlay, Rect(Point(0, 180), histSize));
            cv::bitwise_or(t, histo, t);
        }

    };
}

