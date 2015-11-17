
#pragma once

#include "stdafx.h"

using namespace std;

namespace openCVGraph
{
    struct ZoomWindowPosition
    {
        int x = 0;              // center of image
        int y = 0;
        int dx = 0;             // delta from center due to mouse dragging
        int dy = 0;
        int zoomFactor = 0;
    };

    struct GraphProperty
    {
        GraphProperty();
        GraphProperty(const std::string &, const boost::any &);

        std::string Name;
        boost::any Value;
    };

    typedef std::list<GraphProperty> GraphProperties;

#define MAX_ZOOMVIEW_LOCKS 10

	class  GraphData {
	public:
		std::string m_GraphName;			// Name of the loop processor running this graph
		bool m_AbortOnESC;                  // Exit the graph thread if ESC is hit?

		cv::Mat m_imCapture;                // Capture image.  Don't modify this except in capture filters.
        cv::Mat m_imCapture8U;              // 8 bit monochrome version
        cv::Mat m_imResult;                 // "Result" image.  Could be anything.  Capture filters just copy imCapture to imResult.

        // Cuda!
        bool m_UseCuda = true;
        cv::cuda::GpuMat m_imCaptureGpu16U;
        cv::cuda::GpuMat m_imCaptureGpu32F;
        cv::cuda::GpuMat m_imCaptureGpu8U;

        // Bag for random data
        GraphProperties m_Properties;
        
		// std::vector<cv::Mat> m_imStack;      // "Stack" of images used by cooperating filters.
		int m_FrameNumber = 0;                  // Current frame being processed.

        // Lock zoom and scroll positions of different filters
        ZoomWindowPosition ZoomWindowPositions[MAX_ZOOMVIEW_LOCKS];     // Lock ZoomWindows

        // Return property or null if not found
        //void* GetProperty(const string name) {
        //    auto it = std::find_if(std::begin(m_Properties),
        //        std::end(m_Properties),
        //        [&](const GraphProperty prop) { return prop.Name == name; });

        //    if (m_Properties.end() == it)
        //    {
        //        return NULL;
        //    }
        //    else
        //    {
        //        const int pos = std::distance(m_Properties.begin(), it) + 1;
        //        std::cout << "item found at position " << pos << std::endl;
        //        return (void *) (((GraphProperty*)*it)->value);
        //    }
        //}

        // Return property or null if not found
        //void* SetProperty(const string name, ) {
        //    auto it = std::find_if(std::begin(m_Properties),
        //        std::end(m_Properties),
        //        [&](const GraphProperty prop) { return prop.Name == name; });

        //    if (m_Properties.end() == it)
        //    {
        //        return NULL;
        //    }
        //    else
        //    {
        //        const int pos = std::distance(m_Properties.begin(), it) + 1;
        //        std::cout << "item divisible by 17 found at position " << pos << std::endl;
        //    }
        //}


	};
}
