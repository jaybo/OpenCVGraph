#pragma once

#ifndef INCLUDE_OCVG_DELAY
#define INCLUDE_OCVG_DELAY

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // ---------------------------------------------------------------------------------------
    // Delay filter whose only purpose in life is to verify graph operation during development
    // ---------------------------------------------------------------------------------------

    class Delay : public Filter
    {
    public:

        Delay::Delay(std::string name, GraphData& graphData,
            int sourceFormat = -1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }

        //Allocate resources if needed, and specify the image formats required
        bool Delay::init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);
            return true;
        }

        // Do all of the work here.
        ProcessResult Delay::process(GraphData& graphData) override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayMS));

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        // View updates are a separate function so they don't 
        // affect timing of the main process function
        void Delay::processView(GraphData& graphData) override
        {
        }

        void  Delay::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "delayMS" << m_delayMS;
        }

        void  Delay::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["delayMS"] >> m_delayMS;
        }

    private:
        int m_delayMS = 10;

    };
}
#endif