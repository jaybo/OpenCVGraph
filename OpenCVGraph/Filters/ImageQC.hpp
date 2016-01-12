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
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }


        //Allocate resources if needed
        bool ImageQC::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            if (m_Enabled) {
                // Advertise the format(s) we need
                graphData.m_CommonData->m_NeedCV_8UC1 = true;
            }
            return true;
        }

        // deallocate resources
        bool ImageQC::fini(GraphData& graphData) override
        {
            return true;
        }

        ProcessResult ImageQC::process(GraphData& graphData) override
        {
            if (m_UseCuda) {
                cv::cuda::minMax(graphData.m_CommonData->m_imCapGpu16UC1, &m_dCapMin, &m_dCapMax);
                cv::cuda::calcHist(graphData.m_CommonData->m_imCapGpu8UC1, m_histogram);
                
                // This version fails!!!
                // cv::cuda::histEven(graphData.m_CommonData->m_imCapGpu16UC1, m_histogram, 256, 0, UINT16_MAX);

                auto nPoints = graphData.m_CommonData->m_imCapGpu16UC1.size().area();
                m_Mean = (int) (cv::cuda::sum(graphData.m_CommonData->m_imCapGpu16UC1)[0] / nPoints);
            }
            else {
                abort();
            }
            return ProcessResult::OK;
        }



        void  ImageQC::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "use_Cuda" << m_UseCuda;
        }

        void  ImageQC::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["use_Cuda"] >> m_UseCuda;
        }

        // ITemcaQC
        QCInfo ImageQC::getQCInfo() {
            QCInfo info;

            Mat hist(m_histogram); // copies from gpu

            const int32_t * p = hist.ptr<int32_t>(0);

            for (int i = 0; i < hist.cols; i++) {
                info.histogram[i] = p[i];
            }
            info.max_value = (int) m_dCapMax;
            info.min_value = (int) m_dCapMin;
            info.mean_value = m_Mean;
            return info;
        }

    private:
        int m_Mean;
        cv::cuda::GpuMat m_histogram;
        double m_dCapMax, m_dCapMin;
        bool m_UseCuda = true;


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

        void ImageQC::DrawOverlay(GraphData graphData) {
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

