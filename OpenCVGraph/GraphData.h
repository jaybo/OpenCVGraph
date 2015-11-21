
#pragma once

#include "stdafx.h"

using namespace std;
using namespace spdlog;

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
        GraphProperty(const std::string &, const void *);

        std::string Name;
        void * Value;
    };

    typedef std::list<GraphProperty> GraphProperties;

    // Result of calling "process()" on each filter
    enum ProcessResult {
        OK,             // Normal result, continue through the processing loop
        Abort,          // Catastropic error, abort
        Continue,       // GoTo beginning of the loop.  Averaging filters will issue this result until they've accumulated enough images.
    };


#define MAX_ZOOMVIEW_LOCKS 10

	class  GraphData {
	public:
        GraphData() {
            m_Logger = spdlog::stdout_logger_mt("console");
        }

		std::string m_GraphName;			// Name of the loop processor running this graph
		bool m_AbortOnESC;                  // Exit the graph thread if ESC is hit?

        // Filters tell everybody what formats they need
        bool m_NeedCV_8UC1 = false;
        bool m_NeedCV_8UC3 = false;
        bool m_NeedCV_16UC1 = false;
        bool m_NeedCV_32FC1 = false;

        // And what formats they need for Results
        //bool m_NeedResult8U = false;
        //bool m_NeedResult16U = false;
        //bool m_NeedResult32F = false;

		cv::Mat m_imCapture;                // Raw Capture image.  Don't modify this except in capture filters.

        cv::Mat m_imCapture8U;              // 8 bit monochrome version if m_NeedCV_8UC1 is true
        cv::Mat m_imCapture16U;             // 16 bit monochrome version
        cv::Mat m_imCapture32F;             // 32 bit monochrome version
        cv::Mat m_imCapture8UC3;            // 24 bit RGB version

        cv::Mat m_imResult8U;               // "Result" image.  Could be anything.  Capture filters just copy imCapture to imResult.
        cv::Mat m_imResult16U;                 
        cv::Mat m_imResult32F;                 
        cv::Mat m_imResult8UC3;             // 24 bit RGB version

        // Cuda!
        bool m_UseCuda = true;
        cv::cuda::GpuMat m_imCaptureGpu8U;
        cv::cuda::GpuMat m_imCaptureGpu16U;
        cv::cuda::GpuMat m_imCaptureGpu32F;

        cv::cuda::GpuMat m_imResultGpu8U;
        cv::cuda::GpuMat m_imResultGpu16U;
        cv::cuda::GpuMat m_imResultGpu32F;

        // Bag for random data
        GraphProperties m_Properties;
        
		// std::vector<cv::Mat> m_imStack;      // "Stack" of images used by cooperating filters.
		int m_FrameNumber = 0;                  // Current frame being processed.

        // Lock zoom and scroll positions of different filters
        ZoomWindowPosition ZoomWindowPositions[MAX_ZOOMVIEW_LOCKS];     // Lock ZoomWindows

        std::shared_ptr<logger> m_Logger;

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
