#include "..\stdafx.h"

using namespace std;
using namespace cv;
namespace fs = ::boost::filesystem;

#include "..\FrameProcessor.h"
#include "FPRunningStats.h"

namespace openCVGui
{
    // Calculate and display mean and standard deviation while streaming.  See:
    //
    //    http://www.johndcook.com/blog/standard_deviation/
    //

    FPRunningStats::FPRunningStats(std::string name, GraphData& graphData, bool showView)
        : FrameProcessor(name, graphData, showView)
    {
    }

    //Allocate resources if needed
    bool FPRunningStats::init(GraphData& graphData)
    {
		// call the base to read/write configs
		FrameProcessor::init(graphData);
        m_n = 0;
		return true;
    }

    bool FPRunningStats::process(GraphData& graphData)
    {
        firstTime = false;
		bool fOK = true;

        // let the camera stabilize
        if (graphData.frameCounter < 10) return true;

        m_n++;

        cv::Mat capF;
        graphData.imCapture.convertTo(capF, CV_32F);

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

        Calc(graphData);

        graphData.imCapture.copyTo(imView);
        imView = imView * 16;
        cv::resize(imView, imView, cv::Size(512, 512));

        std::ostringstream str;
        str << "mean: " << dMean << " std: " << dStdDevMean;
        cv::putText(imView, str.str(), Point(20, 50), CV_FONT_HERSHEY_DUPLEX, 0.8, CV_RGB(255, 255, 255));

        str.str("");
        str << "minStd: " << dStdDevMin << " maxStd: " << dStdDevMax;
        cv::putText(imView, str.str(), Point(20, 90), CV_FONT_HERSHEY_DUPLEX, 0.66, CV_RGB(255, 255, 255));

        str.str("");
        str << "capMin: " << (int) dCapMin << " capMax: " << (int) dCapMax;
        cv::putText(imView, str.str(), Point(20, 120), CV_FONT_HERSHEY_DUPLEX, 0.66, CV_RGB(255, 255, 255));

        cv::imshow(CombinedName, imView);
        return fOK;
    }

    // deallocate resources
    bool FPRunningStats::fini(GraphData& graphData)
    {
		return true;
    }




    void  FPRunningStats::saveConfig() 
    {
        FileStorage fs2(persistFile, FileStorage::WRITE);
        fs2 << "tictoc" << tictoc;

        fs2.release();
    }

    void  FPRunningStats::loadConfig()
    {
        FileStorage fs2(persistFile, FileStorage::READ);
		cout << persistFile << endl;

        fs2["tictoc"] >> tictoc;

        fs2.release();
    }
}
