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
            m_showView = true;
            if (m_Enabled) {
                // Define the image formats we use:
                graphData.m_CommonData->m_NeedCV_16UC1 = true;
            }
            return true;
        }

        // Do all of the work here.
        ProcessResult Simple::process(GraphData& graphData) override
        {
            // shift 12 bit images up to full 16 bit resolution
            // graphData.m_imOut16UC1 = 4 * graphData.m_CommonData->m_imCap16UC1;

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        // View updates are a separate function so they don't 
        // affect timing of the main process function
        void Simple::processView(GraphData& graphData) override
        {
            if (m_showView) {
                graphData.m_imOut16UC1.convertTo(graphData.m_imOut8UC1, CV_8UC1, 1 / 256.0f);
                graphData.m_imOut8UC1.copyTo(m_imView);
                Filter::processView(graphData);
            }
        }
    };
}
#endif