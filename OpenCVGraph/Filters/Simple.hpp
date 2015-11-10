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
            : Filter(name, graphData, showView, width, height)
        {
        }

        bool Simple::process(GraphData& graphData)
        {
            // do something random to the capture data
            graphData.m_imResult = graphData.m_imCapture * 2;

            if (m_showView) {
                // Assumes nobody downstream will muck with imResult
                m_imView = graphData.m_imResult;  
                
                // If downstream filters DO muck with imResult, we need to copy as in:
                // graphData.m_imResult.copyTo(m_imView);
            }
            return true;  // if you return false, the graph stops
        }
    };
}
#endif