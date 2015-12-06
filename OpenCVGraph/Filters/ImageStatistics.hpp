
#pragma once

#ifndef INCLUDE_OCVG_IMAGESTATISTICS_HPP
#define INCLUDE_OCVG_IMAGESTATISTICS_HPP

#include "..\stdafx.h"

using namespace cv;

namespace openCVGraph
{
    // http://www.johndcook.com/blog/standard_deviation/


    // This currently only works on m_imCapture.  It should be modified to use m_imOut...

    class ImageStatistics : public Filter
    {
    public:
        ImageStatistics(std::string name, GraphData& graphData, 
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }


        //Allocate resources if needed
        bool ImageStatistics::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            if (m_Enabled) {
                // Advertise the format(s) we need
                graphData.m_NeedCV_32FC1 = true;

                m_N = 0;
            }
            return true;
        }

        // deallocate resources
        bool ImageStatistics::fini(GraphData& graphData) override
        {
            return true;
        }

        bool ImageStatistics::processKeyboard(GraphData& data, int key) override
        {
            bool fOK = true;
            if (key == ' ') {
                m_N = 0;    // RESET STATISTICS IF THE SPACE KEY IS PRESSED
            }
            if (m_showView) {
                return m_ZoomView.KeyboardProcessor(key);
            }
            return fOK;
        }

        void  ImageStatistics::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "use_Cuda" << m_UseCuda;
        }

        void  ImageStatistics::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["use_Cuda"] >> m_UseCuda;
        }


    private:
        int m_N;
        bool m_UseCuda = true;
        double dCapMax, dCapMin;
        double dMean, dMeanMin, dMeanMax, dStdDevMean, dStdDevMin, dStdDevMax, dVarMin, dVarMax;
        cv::Mat m_oldM, m_newM, m_oldS, m_newS, m_capF, m_imVariance, m_dOld, m_dNew;

        cv::cuda::GpuMat m_oldMGpu, m_newMGpu, m_oldSGpu, m_imVarianceGpu, m_newSGpu, m_dOldGpu, m_dNewGpu, m_TGpu;


        void ImageStatistics::Accumulate(GraphData& graphData) {
            graphData.m_imOut8UC1.convertTo(m_capF, CV_32F);

            // See Knuth TAOCP vol 2, 3rd edition, page 232
            if (m_N == 1)
            {
                m_capF.copyTo(m_newM);
                m_capF.copyTo(m_oldM);
                m_oldS = m_oldM.mul(Scalar(0.0));
            }
            else
            {
                m_dOld = m_capF - m_oldM;
                m_dNew = m_capF - m_newM;
                m_newM = m_oldM + m_dOld / m_N;
                m_newS = m_oldS + m_dOld.mul(m_dNew);

                // set up for next iteration
                m_newM.copyTo(m_oldM);
                m_newS.copyTo(m_oldS);
            }
        }

        void ImageStatistics::AccumulateGpu(GraphData& graphData) {
            if (m_N == 1)
            {
                graphData.m_imOutGpu32FC1.copyTo(m_newMGpu);
                graphData.m_imOutGpu32FC1.copyTo(m_oldMGpu);
                cuda::multiply(m_oldMGpu, Scalar(0.0), m_oldSGpu);  // m_oldSGpu = m_oldMGpu * 0.0;
            }
            else
            {
                cuda::subtract(graphData.m_imOutGpu32FC1, m_oldMGpu, m_dOldGpu); //cv::Mat dOld = m_capF - m_oldM;
                cuda::subtract(graphData.m_imOutGpu32FC1, m_newMGpu, m_dNewGpu); //cv::Mat dNew = m_capF - m_newM;

                cuda::divide(m_dOldGpu, Scalar(m_N), m_TGpu);   // Need a temp here m_TGpu
                cuda::add(m_oldMGpu, m_TGpu, m_newMGpu);
                cuda::multiply(m_dOldGpu, m_dNewGpu, m_TGpu);   // temp again
                cuda::add(m_oldSGpu, m_TGpu, m_newSGpu);

                // set up for next iteration
                m_newMGpu.copyTo(m_oldMGpu);
                m_newSGpu.copyTo(m_oldSGpu);
            }
        }

        void ImageStatistics::Calc(GraphData& graphData)
        {
            cv::Point ptMin, ptMax;
            if (graphData.m_imCapture.channels() > 1) {
                cv::Mat gray;
                cv::cvtColor(graphData.m_imCapture, gray, CV_BGR2GRAY);
                cv::minMaxLoc(gray, &dCapMin, &dCapMax, &ptMin, &ptMax);
            }
            else {
                cv::minMaxLoc(graphData.m_imCapture, &dCapMin, &dCapMax, &ptMin, &ptMax);
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

        void ImageStatistics::CalcGpu(GraphData& graphData)
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

        ProcessResult ImageStatistics::process(GraphData& graphData)
        {
            m_firstTime = false;

            // let the camera stabilize
            // if (graphData.m_FrameNumber < 2) return true;

            m_N++;  // count of frames processed

            m_UseCuda ? AccumulateGpu(graphData) : Accumulate(graphData);

            if (m_N >= 2) {
                m_UseCuda ? CalcGpu(graphData) : Calc(graphData);
            }
            return ProcessResult::OK;
        }

        void ImageStatistics::processView(GraphData& graphData)
        {
            if (m_showView) {
                // Convert back to 8 bits for the view
                if (graphData.m_imCapture.depth() == CV_16U) {
                    graphData.m_imCapture.convertTo(m_imView, CV_8UC1, 1.0 / 256);
                }
                else {
                    m_imView = graphData.m_imCapture;
                }
                if (m_N >= 2) {
                    DrawOverlay(graphData);
                }
                Filter::processView(graphData);
            }
        }

        void ImageStatistics::DrawOverlay(GraphData graphData) {
            ClearOverlayText();

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "         min   mean   max";
            DrawOverlayTextMono(str.str(), Point(posLeft, 30), scale);

            str.str("");
            str << "Cap:" << std::setfill(' ') << setw(7) << (int)dCapMin << setw(7) << (int)dMean << setw(7) << (int)dCapMax;
            DrawOverlayTextMono(str.str(), Point(posLeft, 70), scale);

            str.str("");
            str << "SD: " << std::setfill(' ') << setw(7) << (int)dStdDevMin << setw(7) << (int)dStdDevMean << setw(7) << (int)dStdDevMax;
            DrawOverlayTextMono(str.str(), Point(posLeft, 120), scale);

            str.str("");
            str << "SPACE to reset";
            DrawOverlayTextMono(str.str(), Point(posLeft, 460), scale);

            str.str("");
            str << m_N << "/" << graphData.m_FrameNumber;
            DrawOverlayTextMono(str.str(), Point(posLeft, 500), scale);

            auto histSize = Size(512, 200);
            Mat histo = createGrayHistogram(graphData.m_imCapture, 256, histSize.width, histSize.height);

            Mat t = Mat(m_imViewTextOverlay, Rect(Point(0, 180), histSize));
            cv::bitwise_or(t, histo, t);
        }

    };
}

#endif