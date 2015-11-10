#include "..\stdafx.h"
#include "FPRunningStats.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
    // Calculate and display mean and standard deviation while streaming.  See:
    //
    //    http://www.johndcook.com/blog/standard_deviation/
    //

    FPRunningStats::FPRunningStats(std::string name, GraphData& graphData, 
        bool showView, int width, int height)
        : Filter(name, graphData, showView, width, height)
    {
    }

    // 
    bool FPRunningStats::processKeyboard(GraphData& data, int key)
    {
        bool fOK = true;
        if (key == ' ') {
            m_n = 0;    // RESET
        }
        if (m_showView) {
            return view.KeyboardProcessor(key);  
        }
        return fOK;
    }


    //Allocate resources if needed
    bool FPRunningStats::init(GraphData& graphData)
    {
        // call the base to read/write configs
        Filter::init(graphData);
        m_n = 0;
        return true;
    }

    void FPRunningStats::Calc(GraphData& graphData)
    {
//#define GPU
#ifdef GPU
        cuda::GpuMat im_gpu1;
        im_gpu1.upload(graphData.m_imCapture); //allocate memory and upload to GPU
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

    bool FPRunningStats::process(GraphData& graphData)
    {
        m_firstTime = false;
        bool fOK = true;

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

            graphData.m_imCapture.copyTo(m_imView);
            m_imView = m_imView * 16;
            cv::resize(m_imView, m_imView, cv::Size(512, 512));

            std::ostringstream str;

            str.str("");
            str << "SPACE to reset";
            DrawShadowTextMono(m_imView, str.str(), Point(20, 20), 0.66);

            str.str("");
            str << "mean: " << setiosflags(ios::fixed) << setprecision(1) << dMean << " std: " << dStdDevMean;
            DrawShadowTextMono(m_imView, str.str(), Point(20, 50), 0.8);

            str.str("");
            str << "capMin: " << (int)dCapMin << " capMax: " << (int)dCapMax;
            DrawShadowTextMono(m_imView, str.str(), Point(20, 100), 0.66);


            str.str("");
            str << "minStd: " << dStdDevMin << " maxStd: " << dStdDevMax;
            DrawShadowTextMono(m_imView, str.str(), Point(20, 120), 0.66);

            str.str("");
            str << m_n << "/" << graphData.m_FrameNumber;
            DrawShadowTextMono(m_imView, str.str(), Point(20, 500), 0.66);

            // vector<Mat> histo = createHistogramImages(graphData.m_imCapture);

            cv::imshow(m_CombinedName, m_imView);
        }
        return fOK;
    }

    // deallocate resources
    bool FPRunningStats::fini(GraphData& graphData)
    {
        return true;
    }




    void  FPRunningStats::saveConfig(FileStorage& fs, GraphData& data)
    {
    }

    void  FPRunningStats::loadConfig(FileNode& fs, GraphData& data)
    {
    }
}
