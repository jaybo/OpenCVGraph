
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"

namespace fs = ::boost::filesystem;

#include "..\Property.h"
#include "..\GraphData.h"
#include "..\Config.h"
#include "..\FrameProcessor.h"
#include "..\ZoomView.h"

namespace openCVGui
{
    // http://www.johndcook.com/blog/standard_deviation/

    class FPRunningStats : public FrameProcessor
    {
    public:
        FPRunningStats(std::string name, GraphData& data, bool showView = false);

        virtual bool init(GraphData& graphData) override;
        virtual bool process(GraphData& graphData) override;
        virtual bool fini(GraphData& graphData) override;

        virtual void saveConfig() override;
        virtual void loadConfig() override;

        int NumDataValues() const
        {
            return m_n;
        }

        void Calc(GraphData& graphData)
        {
            cv::minMaxLoc(graphData.imCapture, &dCapMin, &dCapMax);

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

        }

    private:
        int m_n;
        double dCapMax, dCapMin;
        double dMean, dMeanMin, dMeanMax, dStdDevMean, dStdDevMin, dStdDevMax, dVarMin, dVarMax;
        cv::Mat m_oldM, m_newM, m_oldS, m_newS;

    };
}