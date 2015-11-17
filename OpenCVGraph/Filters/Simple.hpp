#pragma once

#ifndef INCLUDE_SIMPLE
#define INCLUDE_SIMPLE

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Simplest possible filter
    class Simple : public Filter
    {
    public:

        Simple::Simple(std::string name, GraphData& graphData,
            bool showView = true, int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool Simple::process(GraphData& graphData)
        {
            // do something random to the capture data and put the result into imResult
            graphData.m_imResult = 2 *graphData.m_imCapture;

            if (m_showView) {
                graphData.m_imResult.copyTo(m_imView);
            }
            return true;  // if you return false, the graph stops
        }
    };
}
#endif