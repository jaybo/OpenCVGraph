#pragma once

#ifndef INCLUDE_OCVG_SIMPLE
#define INCLUDE_OCVG_SIMPLE

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

        //Allocate resources if needed, and specify the image formats required
        bool Simple::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);

            // Define the image formats we use:
            graphData.m_NeedCV_8UC1 = true;

            // To write on the Text overlay, you must allocate it.
            // This indicates to the renderer the need to merge it with the final output image.
            // m_imViewTextOverlay = Mat(m_width, m_height, CV_8U);

            return true;
        }

        // Do all of the work here.
        ProcessResult Simple::process(GraphData& graphData)
        {
            // do something random to imResult
            graphData.m_imResult8U = 2 *graphData.m_imResult8U;

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        // View updates are a separate function so they don't 
        // affect timing of the main processing loop
        void Simple::processView(GraphData& graphData)
        {
            if (m_showView) {
                graphData.m_imResult8U.copyTo(m_imView);
                Filter::processView(graphData);
            }
        }
    };
}
#endif