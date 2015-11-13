
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
        ImageStatistics(std::string name, GraphData& graphData, bool showView, int width = 512, int height = 512)
            : Filter(name, graphData, showView, width, height)
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
                m_n = 0;    // RESET
            }
            if (m_showView) {
                return m_ZoomView.KeyboardProcessor(key);
            }
            return fOK;
        }

        void  ImageStatistics::saveConfig(FileStorage& fs, GraphData& data)
        {
        }

        void  ImageStatistics::loadConfig(FileNode& fs, GraphData& data)
        {
        }

        void ImageStatistics::Calc(GraphData& graphData)
        {
            //#define GPU
#ifdef GPU



#else
            cv::Point ptMin, ptMax;
            if (graphData.m_imCapture.channels() > 1) {
                cv::Mat gray;
                cvtColor(graphData.m_imCapture, gray, CV_BGR2GRAY);
                cv::minMaxLoc(gray, &dCapMin, &dCapMax, &ptMin, &ptMax);

            }
            else {
                cv::minMaxLoc(graphData.m_imCapture, &dCapMin, &dCapMax, &ptMin, &ptMax);
            }

            cv::Mat imVariance;
            cv::Scalar imMean, imStdDev;
            cv::meanStdDev(m_newM, imMean, imStdDev);   // stdDev here is across the mean image
            dMean = imMean[0];
            // Mean, and min and max of mean image
            cv::minMaxLoc(m_newM, &dMeanMin, &dMeanMax);

            // Variance
            imVariance = m_newS / (m_n - 1);
            cv::Scalar varMean, varStd;
            cv::meanStdDev(imVariance, varMean, varStd);
            cv::minMaxLoc(imVariance, &dVarMin, &dVarMax);
            dStdDevMean = sqrt(varMean[0]);
            dStdDevMin = sqrt(dVarMin);
            dStdDevMax = sqrt(dVarMax);
#endif
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

            cv::Mat capF;
            graphData.m_imCapture.convertTo(capF, CV_32F);

            // See Knuth TAOCP vol 2, 3rd edition, page 232
            if (m_n == 1)
            {
                capF.copyTo(m_newM);
                capF.copyTo(m_oldM);
                m_oldS = 0.0 * m_oldM;
            }
            else
            {
                cv::Mat dOld = capF - m_oldM;
                cv::Mat dNew = capF - m_newM;
                m_newM = m_oldM + dOld / m_n;
                m_newS = m_oldS + dOld.mul(dNew);

                // set up for next iteration
                m_newM.copyTo(m_oldM);
                m_newS.copyTo(m_oldS);
            }

            if (m_n >= 2) {
                Calc(graphData);

                m_imViewOverlay = 0;
                std::ostringstream str;

                int posLeft = 10;
                double scale = 1.0;

                str.str("");
                str << "      min    mean   max";
                DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 50), scale);

                str.str("");
                str << "Cap:"  << std::setfill(' ') << setw(7) << (int) dCapMin << setw(7) << (int)dMean << setw(7) << (int)dCapMax;
                DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 100), scale);

                str.str("");
                str << "SD: "  << std::setfill(' ') << setw(7) <<  (int)dStdDevMin << setw(7) <<  (int)dStdDevMean << setw(7) << (int)dStdDevMax;
                DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 150), scale);

                str.str("");
                str << "SPACE to reset";
                DrawShadowTextMono(m_imViewOverlay, str.str(), Point(posLeft, 400), scale);

                str.str("");
                str << m_n << "/" << graphData.m_FrameNumber;
                DrawShadowTextMono(m_imViewOverlay, str.str(), Point(20, 500), scale);

                
                // vector<Mat> histo = createHistogramImages(graphData.m_imCapture);

            }
            return fOK;
        }

    private:
        int m_n;
        double dCapMax, dCapMin;
        double dMean, dMeanMin, dMeanMax, dStdDevMean, dStdDevMin, dStdDevMax, dVarMin, dVarMax;
        cv::Mat m_oldM, m_newM, m_oldS, m_newS;

        cv::cuda::GpuMat m_oldMGpu, m_newMGpu, m_oldSGpu, m_newSGpu;

    };
}