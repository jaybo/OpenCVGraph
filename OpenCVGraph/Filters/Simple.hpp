#pragma once

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
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        //Allocate resources if needed, and specify the image formats required
        bool Simple::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            bool fOK = Filter::init(graphData);

            // do setup or allocations here
            return fOK;
        }

        // Do all of the work here.
        ProcessResult Simple::process(GraphData& graphData) override
        {
            graphData.EnsureFormatIsAvailable(false, CV_16UC1, false);
            graphData.m_CommonData->m_imCapture16UC1.copyTo(graphData.m_imOut);

            // shift 12 bit images up to full 16 bit resolution
            // graphData.m_imOut16UC1 = 4 * graphData.m_CommonData->m_imOut;

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        // View updates are a separate function so they don't 
        // affect timing of the main process function
        void Simple::processView(GraphData& graphData) override
        {
            if (m_showView) {
                m_imView = graphData.m_imOut;
                Filter::processView(graphData);
            }
        }
    };
}
