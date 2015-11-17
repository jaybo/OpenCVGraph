
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"

using namespace cv;

namespace openCVGraph
{
    // http://www.johndcook.com/blog/standard_deviation/

    class ImageStatistics : public Filter
    {
    public:
        ImageStatistics(std::string name, GraphData& graphData, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
            // To write on the overlay, you must allocate it.
            // This indicates to the renderer the need to merge it with the final output image.
            m_imViewOverlay = Mat(height, width, CV_8U);
        }


        //Allocate resources if needed
        bool ImageStatistics::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            m_n = 0;
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
                m_n = 0;    // RESET STATISTICS IF THE SPACE KEY IS PRESSED
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
        int m_n;
        bool m_UseCuda = true;
        double dCapMax, dCapMin;
        double dMean, dMeanMin, dMeanMax, dStdDevMean, dStdDevMin, dStdDevMax, dVarMin, dVarMax;
        cv::Mat m_oldM, m_newM, m_oldS, m_newS, m_capF, m_imVariance, m_dOld, m_dNew;

        cv::cuda::GpuMat m_oldMGpu, m_newMGpu, m_oldSGpu, m_imVarianceGpu, m_newSGpu, m_dOldGpu, m_dNewGpu, m_TGpu;


        void ImageStatistics::Accumulate(GraphData& graphData) {
            graphData.m_imCapture.convertTo(m_capF, CV_32F);

            // See Knuth TAOCP vol 2, 3rd edition, page 232
            if (m_n == 1)
            {
                m_capF.copyTo(m_newM);
                m_capF.copyTo(m_oldM);
                m_oldS = m_oldM.mul(Scalar(0.0));
            }
            else
            {
                m_dOld = m_capF - m_oldM;
                m_dNew = m_capF - m_newM;
                m_newM = m_oldM + m_dOld / m_n;
                m_newS = m_oldS + m_dOld.mul(m_dNew);

                // set up for next iteration
                m_newM.copyTo(m_oldM);
                m_newS.copyTo(m_oldS);
            }
        }

        void ImageStatistics::AccumulateGpu(GraphData& graphData) {
            if (m_n == 1)
            {
                graphData.m_imCaptureGpu32F.copyTo(m_newMGpu);
                graphData.m_imCaptureGpu32F.copyTo(m_oldMGpu);
                cuda::multiply(m_oldMGpu, Scalar(0.0), m_oldSGpu);  // m_oldSGpu = m_oldMGpu * 0.0;
            }
            else
            {
                cuda::subtract(graphData.m_imCaptureGpu32F, m_oldMGpu, m_dOldGpu); //cv::Mat dOld = m_capF - m_oldM;
                cuda::subtract(graphData.m_imCaptureGpu32F, m_newMGpu, m_dNewGpu); //cv::Mat dNew = m_capF - m_newM;

                cuda::divide(m_dOldGpu, Scalar(m_n), m_TGpu);   // Need a temp here m_TGpu
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
                cvtColor(graphData.m_imCapture, gray, CV_BGR2GRAY);
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
            m_imVariance = m_newS / (m_n - 1);
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
            cv::cuda::minMaxLoc(graphData.m_imCaptureGpu32F, &dCapMin, &dCapMax, &ptMin, &ptMax);

            cv::Scalar sMean, sStdDev;
            // argh, only works with 8pp!!!
            // cv::cuda::meanStdDev(m_newMGpu, imMean, imStdDev);   // stdDev here is across the mean image
            sMean = cv::cuda::sum(m_newMGpu);
            auto nPoints = graphData.m_imCaptureGpu32F.size().area();
            dMean = sMean[0] / nPoints;

            // Mean, and min and max of mean image
            cv::cuda::minMaxLoc(m_newMGpu, &dMeanMin, &dMeanMax, &ptMin, &ptMax);

            // Variance
            cuda::divide(m_newSGpu, Scalar(m_n - 1), m_imVarianceGpu);  // imVariance = m_newS / (m_n - 1);
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

        bool ImageStatistics::process(GraphData& graphData)
        {
            m_firstTime = false;
            bool fOK = true;

            if (m_showView) {
                m_imView = graphData.m_imCapture8U;
            }

            // let the camera stabilize
            if (graphData.m_FrameNumber < 2) return true;

            m_n++;

            m_UseCuda ? AccumulateGpu(graphData) : Accumulate(graphData);

            if (m_n >= 2) {
                m_UseCuda ? CalcGpu(graphData) : Calc(graphData);
                DrawOverlay(graphData);
            }
            return fOK;
        }

        void ImageStatistics::DrawOverlay(GraphData graphData) {
            m_imViewOverlay = 0;
            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "      min    mean   max";
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << "Cap:" << std::setfill(' ') << setw(7) << (int)dCapMin << setw(7) << (int)dMean << setw(7) << (int)dCapMax;
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 100), scale);

            str.str("");
            str << "SD: " << std::setfill(' ') << setw(7) << (int)dStdDevMin << setw(7) << (int)dStdDevMean << setw(7) << (int)dStdDevMax;
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 150), scale);

            str.str("");
            str << "SPACE to reset";
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 400), scale);

            str.str("");
            str << m_n << "/" << graphData.m_FrameNumber;
            DrawShadowTextMono(m_imViewOverlay, str.str(), Point(20, 500), scale);

            // vector<Mat> histo = createHistogramImages(graphData.m_imCapture);
        }

    };
}