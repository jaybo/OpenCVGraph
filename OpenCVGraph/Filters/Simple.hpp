#pragma once

#ifndef INCLUDE_OCVG_SIMPLE
#define INCLUDE_OCVG_SIMPLE

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Simplest possible filter with a view

    class Simple : public Filter
    {
    public:

        Simple::Simple(std::string name, GraphData& graphData,
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }

        //Allocate resources if needed, and specify the image formats required
        bool Simple::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            if (m_Enabled) {
                // Define the image formats we use:
                graphData.m_NeedCV_8UC1 = true;
            }
            return true;
        }

        // Do all of the work here.
        ProcessResult Simple::process(GraphData& graphData) override
        {
            // do something the one of the Out images
            graphData.m_imOut8UC1 = 2 *graphData.m_imOut8UC1;

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        // View updates are a separate function so they don't 
        // affect timing of the main process function
        void Simple::processView(GraphData& graphData) override
        {
            if (m_showView) {
                graphData.m_imOut8UC1.copyTo(m_imView);
                Filter::processView(graphData);
            }
        }
    };
}
#endif