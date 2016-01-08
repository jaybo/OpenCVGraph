#pragma once

#include "..\stdafx.h"

using namespace cv;

namespace openCVGraph
{
    // http://www.johndcook.com/blog/standard_deviation/


    // This currently only works on m_CommonData->m_imCapture.  It should be modified to use m_imOut...

    class ImageQC : public Filter
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
                graphData.m_CommonData->m_NeedCV_32FC1 = true;
            }
            return true;
        }

        // deallocate resources
        bool ImageQC::fini(GraphData& graphData) override
        {
            return true;
        }

        bool ImageQC::processKeyboard(GraphData& data, int key) override
        {
            bool fOK = true;
            return fOK;
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


    private:
        bool m_UseCuda = true;
        double dCapMax, dCapMin;
        double dMean, dMeanMin, dMeanMax, dStdDevMean, dStdDevMin, dStdDevMax, dVarMin, dVarMax;
        cv::Mat m_oldM, m_newM, m_oldS, m_newS, m_capF, m_imVariance, m_dOld, m_dNew;

        cv::cuda::GpuMat m_oldMGpu, m_newMGpu, m_oldSGpu, m_imVarianceGpu, m_newSGpu, m_dOldGpu, m_dNewGpu, m_TGpu;



        void ImageQC::Calc(GraphData& graphData)
        {
            cv::Point ptMin, ptMax;
            if (graphData.m_CommonData->m_imCapture.channels() > 1) {
                cv::Mat gray;
                cv::cvtColor(graphData.m_CommonData->m_imCapture, gray, CV_BGR2GRAY);
                cv::minMaxLoc(gray, &dCapMin, &dCapMax, &ptMin, &ptMax);
            }
            else {
                cv::minMaxLoc(graphData.m_CommonData->m_imCapture, &dCapMin, &dCapMax, &ptMin, &ptMax);
            }

            cv::Scalar sMean, sStdDev;
            cv::meanStdDev(m_newM, sMean, sStdDev);   // stdDev here is across the mean image
            dMean = sMean[0];
            // Mean, and min and max of mean image
            cv::minMaxLoc(m_newM, &dMeanMin, &dMeanMax);

            // Variance
            m_imVariance = m_newS / (m_N - 1);
            cv::Scalar varMean, varStd;
            cv::meanStdDev(m_imVariance, varMean, varStd);
            cv::minMaxLoc(m_imVariance, &dVarMin, &dVarMax);
            dStdDevMean = sqrt(varMean[0]);
            dStdDevMin = sqrt(dVarMin);
            dStdDevMax = sqrt(dVarMax);

        }

        void ImageQC::CalcGpu(GraphData& graphData)
        {
            cv::Point ptMin, ptMax;
            cv::cuda::minMaxLoc(graphData.m_imOutGpu32FC1, &dCapMin, &dCapMax, &ptMin, &ptMax);

            cv::Scalar sMean, sStdDev;
            // argh, only works with 8pp!!!
            // cv::cuda::meanStdDev(m_newMGpu, imMean, imStdDev);   // stdDev here is across the mean image
            sMean = cv::cuda::sum(m_newMGpu);
            auto nPoints = graphData.m_imOutGpu32FC1.size().area();
            dMean = sMean[0] / nPoints;

            // Mean, and min and max of mean image
            cv::cuda::minMaxLoc(m_newMGpu, &dMeanMin, &dMeanMax, &ptMin, &ptMax);

            // Variance
            cuda::divide(m_newSGpu, Scalar(m_N - 1), m_imVarianceGpu);  // imVariance = m_newS / (m_N - 1);
            cv::Scalar varMean, varStd;
            // argh, only works with 8bpp!!!
            //cv::cuda::meanStdDev(m_imVarianceGpu, varMean, varStd);
            sStdDev = cv::cuda::sum(m_imVarianceGpu);
            double meanVariance = sStdDev[0] / nPoints;

            cv::cuda::minMaxLoc(m_imVarianceGpu, &dVarMin, &dVarMax, &ptMin, &ptMax);
            dStdDevMean = sqrt(meanVariance);
            dStdDevMin = sqrt(dVarMin);
            dStdDevMax = sqrt(dVarMax);
        }

        void ImageQC::processView(GraphData& graphData)
        {
            if (m_showView) {
                // Convert back to 8 bits for the view
                //if (graphData.m_CommonData->m_imCapture.depth() == CV_16U) {
                //    graphData.m_CommonData->m_imCapture.convertTo(m_imView, CV_8UC1, 1.0 / 256);
                //}
                //else {
                //    m_imView = graphData.m_CommonData->m_imCapture;
                //}
                //if (m_N >= 2) {
                //    DrawOverlay(graphData);
                //}
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
            str << "Cap:" << std::setfill(' ') << setw(7) << (int)dCapMin << setw(7) << (int)dMean << setw(7) << (int)dCapMax;
            DrawOverlayText(str.str(), Point(posLeft, 70), scale);

            str.str("");
            str << "SD: " << std::setfill(' ') << setw(7) << (int)dStdDevMin << setw(7) << (int)dStdDevMean << setw(7) << (int)dStdDevMax;
            DrawOverlayText(str.str(), Point(posLeft, 120), scale);

            str.str("");
            str << "SPACE to reset";
            DrawOverlayText(str.str(), Point(posLeft, 460), scale);

            str.str("");
            str << m_N << "/" << graphData.m_FrameNumber;
            DrawOverlayText(str.str(), Point(posLeft, 500), scale);

            auto histSize = Size(512, 200);
            Mat histo = createGrayHistogram(graphData.m_CommonData->m_imCapture, 256, histSize.width, histSize.height);

            Mat t = Mat(m_imViewTextOverlay, Rect(Point(0, 180), histSize));
            cv::bitwise_or(t, histo, t);
        }

    };
}

