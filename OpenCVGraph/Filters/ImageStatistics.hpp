#pragma once

#include "..\stdafx.h"

using namespace cv;

namespace openCVGraph
{

    // -----------------------------------------------------------
    // Keep running statistics on an image stream
    // Mean, Min, Max and Standard Deviation of each 
    // http://www.johndcook.com/blog/standard_deviation/
    // -----------------------------------------------------------

    class ImageStatistics : public Filter
    {
    public:
        ImageStatistics(std::string name, GraphData& graphData, 
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }


        //Allocate resources if needed
        bool ImageStatistics::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            if (m_Enabled) {
                // Advertise the format(s) we need
                // graphData.m_CommonData->m_NeedCV_32FC1 = true;
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
        cv::Mat m_oldM, m_newM, m_oldS, m_newS, m_imVariance, m_dOld, m_dNew;
#ifdef WITH_CUDA
        cv::cuda::GpuMat m_oldMGpu, m_newMGpu, m_oldSGpu, m_imVarianceGpu, m_newSGpu, m_dOldGpu, m_dNewGpu, m_TGpu;
#endif

        void ImageStatistics::Accumulate(GraphData& graphData) {

            // If output stream exists, us it.  Otherwise use capture stream.
            Mat MatSrc = graphData.m_imOut32FC1.empty() ? graphData.m_CommonData->m_imCap32FC1 : graphData.m_imOut32FC1;

            // See Knuth TAOCP vol 2, 3rd edition, page 232
            if (m_N == 1)
            {
                MatSrc.copyTo(m_newM);
                MatSrc.copyTo(m_oldM);
                m_oldS = m_oldM.mul(Scalar(0.0));
            }
            else
            {
                m_dOld = MatSrc - m_oldM;
                m_dNew = MatSrc - m_newM;
                m_newM = m_oldM + m_dOld / m_N;
                m_newS = m_oldS + m_dOld.mul(m_dNew);

                // set up for next iteration
                m_newM.copyTo(m_oldM);
                m_newS.copyTo(m_oldS);
            }
        }

#ifdef WITH_CUDA
        void ImageStatistics::AccumulateGpu(GraphData& graphData) {

            // If output stream exists, us it.  Otherwise use capture stream.
            cuda::GpuMat MatSrc = graphData.m_imOutGpu32FC1.empty() ? graphData.m_CommonData->m_imCapGpu32FC1 : graphData.m_imOutGpu32FC1;

            if (m_N == 1)
            {
                MatSrc.copyTo(m_newMGpu);
                MatSrc.copyTo(m_oldMGpu);
                cuda::multiply(m_oldMGpu, Scalar(0.0), m_oldSGpu);  // m_oldSGpu = m_oldMGpu * 0.0;
            }
            else
            {
                cuda::subtract(MatSrc, m_oldMGpu, m_dOldGpu); //cv::Mat dOld = m_capF - m_oldM;
                cuda::subtract(MatSrc, m_newMGpu, m_dNewGpu); //cv::Mat dNew = m_capF - m_newM;

                cuda::divide(m_dOldGpu, Scalar(m_N), m_TGpu);   // Need a temp here m_TGpu
                cuda::add(m_oldMGpu, m_TGpu, m_newMGpu);
                cuda::multiply(m_dOldGpu, m_dNewGpu, m_TGpu);   // temp again
                cuda::add(m_oldSGpu, m_TGpu, m_newSGpu);

                // set up for next iteration
                m_newMGpu.copyTo(m_oldMGpu);
                m_newSGpu.copyTo(m_oldSGpu);
            }
        }
#endif

        void ImageStatistics::Calc(GraphData& graphData)
        {
            // If output stream exists, us it.  Otherwise use capture stream.
            Mat MatSrc = graphData.m_imOut32FC1.empty() ? graphData.m_CommonData->m_imCap32FC1 : graphData.m_imOut32FC1;

            cv::Point ptMin, ptMax;

            cv::minMaxLoc(MatSrc, &dCapMin, &dCapMax, &ptMin, &ptMax);

            //if (graphData.m_CommonData->m_imCapture.channels() > 1) {
            //    cv::Mat gray;
            //    cv::cvtColor(graphData.m_CommonData->m_imCapture, gray, CV_BGR2GRAY);
            //    cv::minMaxLoc(gray, &dCapMin, &dCapMax, &ptMin, &ptMax);
            //}
            //else {
            //    cv::minMaxLoc(graphData.m_CommonData->m_imCapture, &dCapMin, &dCapMax, &ptMin, &ptMax);
            //}

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
#ifdef WITH_CUDA
        void ImageStatistics::CalcGpu(GraphData& graphData)
        {
            // If output stream exists, us it.  Otherwise use capture stream.
            cuda::GpuMat MatSrc = graphData.m_imOutGpu32FC1.empty() ? graphData.m_CommonData->m_imCapGpu32FC1 : graphData.m_imOutGpu32FC1;

            cv::Point ptMin, ptMax;
            cv::cuda::minMaxLoc(MatSrc, &dCapMin, &dCapMax, &ptMin, &ptMax);

            cv::Scalar sMean, sStdDev;
            sMean = cv::cuda::sum(m_newMGpu);
            auto nPoints = MatSrc.size().area();
            dMean = sMean[0] / nPoints;

            // Mean, and min and max of mean image
            cv::cuda::minMaxLoc(m_newMGpu, &dMeanMin, &dMeanMax, &ptMin, &ptMax);

            // Variance
            cuda::divide(m_newSGpu, Scalar(m_N - 1), m_imVarianceGpu);  // imVariance = m_newS / (m_N - 1);
            cv::Scalar varMean, varStd;
            sStdDev = cv::cuda::sum(m_imVarianceGpu);
            double meanVariance = sStdDev[0] / nPoints;

            cv::cuda::minMaxLoc(m_imVarianceGpu, &dVarMin, &dVarMax, &ptMin, &ptMax);
            dStdDevMean = sqrt(meanVariance);
            dStdDevMin = sqrt(dVarMin);
            dStdDevMax = sqrt(dVarMax);
        }
#endif

        ProcessResult ImageStatistics::process(GraphData& graphData)
        {
            m_firstTime = false;

            graphData.EnsureFormatIsAvailable(m_UseCuda, CV_32FC1);

            // let the camera stabilize
            // if (graphData.m_FrameNumber < 2) return true;

            m_N++;  // count of frames processed

            if (m_UseCuda) {
#ifdef WITH_CUDA
                AccumulateGpu(graphData);
#endif
            }
            else {
                Accumulate(graphData);
            }

            if (m_N >= 2) {
                if (m_UseCuda) {
#ifdef WITH_CUDA
                    CalcGpu(graphData);
#endif
                }
                else {
                    Calc(graphData);
                }
            }
            return ProcessResult::OK;
        }

        void ImageStatistics::processView(GraphData& graphData)
        {
            if (m_showView) {
                //// Convert back to 8 bits for the view
                //if (graphData.m_CommonData->m_imCapture.depth() == CV_16U) {
                //    graphData.m_CommonData->m_imCapture.convertTo(m_imView, CV_8UC1, 1.0 / 256);
                //}
                //else {
                    m_imView = graphData.m_CommonData->m_imCapture;
                //}
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
            DrawOverlayText(str.str(), Point(posLeft, 30), scale, CV_RGB(128,128,128));

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
            cv::cvtColor(histo, histo, CV_GRAY2RGB);

            Mat t = Mat(m_imViewTextOverlay, Rect(Point(0, 180), histSize));
            cv::bitwise_or(t, histo, t);
        }

    };
}

