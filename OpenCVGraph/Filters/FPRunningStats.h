
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"

using namespace cv;

namespace openCVGraph
{
    // http://www.johndcook.com/blog/standard_deviation/

    class FPRunningStats : public Filter
    {
    public:
        FPRunningStats(std::string name, GraphData& data, bool showView, int width = 512, int height = 512);

        virtual bool init(GraphData& graphData) override;
        virtual bool process(GraphData& graphData) override;
        virtual bool fini(GraphData& graphData) override;
        virtual bool processKeyboard(GraphData& data, int key) override;

        virtual void saveConfig(FileStorage fs, GraphData& data) override;
        virtual void loadConfig(FileStorage fs, GraphData& data) override;

        void Calc(GraphData& graphData);

    private:
        int m_n;
        double dCapMax, dCapMin;
        double dMean, dMeanMin, dMeanMax, dStdDevMean, dStdDevMin, dStdDevMax, dVarMin, dVarMax;
        cv::Mat m_oldM, m_newM, m_oldS, m_newS;

    };
}