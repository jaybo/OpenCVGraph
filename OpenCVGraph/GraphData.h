
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
        }

		std::string m_GraphName;			// Name of the loop processor running this graph
		bool m_AbortOnESC;                  // Exit the graph thread if ESC is hit?

        // Filters tell everybody what formats they need
        bool m_NeedCV_8UC1 = false;
        bool m_NeedCV_8UC3 = false;
        bool m_NeedCV_16UC1 = false;
        bool m_NeedCV_32FC1 = false;

        int m_PrimaryImageType;

		cv::Mat m_imCapture;                // Raw Capture image.  Always keep this unmodified

        // Default view
        bool m_NeedCV8UC1View = false;      // forces the m_imOut8UC1 (non-CUDA) to always be used
        

        // Input Mats
        cv::Mat m_imCap8UC3;              
        cv::Mat m_imCap8UC1;              
        cv::Mat m_imCap16UC1;             
        cv::Mat m_imCap32FC1;             

        // Output Mats
        cv::Mat m_imOut8UC3;              
        cv::Mat m_imOut8UC1;              
        cv::Mat m_imOut16UC1;             
        cv::Mat m_imOut32FC1;             

        // Cuda!
        bool m_UseCuda = true;

        cv::cuda::GpuMat m_imCaptureGpu;    // Raw Capture image.  Always keep this unmodified

        // Cuda input Mats
        cv::cuda::GpuMat m_imCapGpu8UC3;
        cv::cuda::GpuMat m_imCapGpu8UC1;
        cv::cuda::GpuMat m_imCapGpu16UC1;
        cv::cuda::GpuMat m_imCapGpu32FC1;

        // Cuda output Mats
        cv::cuda::GpuMat m_imOutGpu8UC3;
        cv::cuda::GpuMat m_imOutGpu8UC1;
        cv::cuda::GpuMat m_imOutGpu16UC1;
        cv::cuda::GpuMat m_imOutGpu32FC1;

        // Bag for random data
        GraphProperties m_Properties;
        
		// std::vector<cv::Mat> m_imStack;      // "Stack" of images used by cooperating filters.
		int m_FrameNumber = 0;                  // Current frame being processed.

        // Lock zoom and scroll positions of different filters
        ZoomWindowPosition ZoomWindowPositions[MAX_ZOOMVIEW_LOCKS];     // Lock ZoomWindows

        std::shared_ptr<logger> m_Logger;

        // A capture or source filter has already put an image into m_imCapture.
        // Now convert it into formats needed by the rest of the graph.
        void CopyCaptureToRequiredFormats()
        {
            int nChannels = m_imCapture.channels();
            int nDepth = m_imCapture.depth();
            int nType = m_imCapture.type();

            if (m_UseCuda) {

                m_imCaptureGpu.upload(m_imCapture);

                switch (nType) {
                case CV_8UC1:
                    if (m_NeedCV_8UC1) {
                        m_imCapGpu8UC1 = m_imCaptureGpu;
                    }
                    if (m_NeedCV_16UC1) {
                        m_imCaptureGpu.convertTo(m_imCapGpu16UC1, CV_16UC1, 256.0);
                    }
                    if (m_NeedCV_32FC1) {
                        m_imCaptureGpu.convertTo(m_imCapGpu32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                    }
                    if (m_NeedCV_8UC3) {
                        m_imCaptureGpu.convertTo(m_imCapGpu8UC3, CV_8UC3);
                    }
                    break;
                case CV_16UC1:
                    if (m_NeedCV_8UC1) {
                        m_imCaptureGpu.convertTo(m_imCapGpu8UC1, CV_8UC1, 1.0 / 256);
                    }
                    if (m_NeedCV_16UC1) {
                        m_imCapGpu16UC1 = m_imCaptureGpu;
                        m_imOut16UC1 = m_imCapture;
                    }
                    if (m_NeedCV_32FC1) {
                        m_imCaptureGpu.convertTo(m_imCapGpu32FC1, CV_32FC1);
                    }
                    if (m_NeedCV_8UC3) {
                        m_imCaptureGpu.convertTo(m_imCapGpu8UC3, CV_8UC3, 1.0 / 256);
                    }
                    break;
                case CV_8UC3:
                    if (m_NeedCV_8UC1 || m_NeedCV_16UC1 || m_NeedCV_32FC1) {
                        cuda::cvtColor(m_imCaptureGpu, m_imCapGpu8UC1, COLOR_RGB2GRAY);
                    }
                    if (m_NeedCV_16UC1) {
                        m_imCapGpu8UC1.convertTo(m_imCapGpu16UC1, CV_16UC1, 256.0);
                    }
                    if (m_NeedCV_32FC1) {
                        m_imCapGpu8UC1.convertTo(m_imCapGpu32FC1, CV_32FC1); // Hmm scale up here?
                    }
                    if (m_NeedCV_8UC3) {
                        m_imCapGpu8UC3 = m_imCaptureGpu;
                    }
                    break;
                default:
                    m_Logger->error("Source format unsupported");
                }

                // And copy to Out Mats
                // ??? should just be reference to Cap Mats

                if (m_NeedCV_8UC1) {
                    m_imCapGpu8UC1.copyTo(m_imOutGpu8UC1);
                }
                if (m_NeedCV_16UC1) {
                    m_imCapGpu16UC1.copyTo(m_imOutGpu16UC1);
                }
                if (m_NeedCV_32FC1) {
                    m_imCapGpu32FC1.copyTo(m_imOutGpu32FC1);
                }
                if (m_NeedCV_8UC3) {
                    m_imCapGpu8UC3.copyTo(m_imOutGpu8UC3);
                }

            }
            else {
                switch (nType) {
                case CV_8UC1:
                    if (m_NeedCV_8UC1) {
                        m_imCap8UC1 = m_imCapture;
                    }
                    if (m_NeedCV_16UC1) {
                        m_imCapture.convertTo(m_imCap16UC1, CV_16UC1, 256.0);
                    }
                    if (m_NeedCV_32FC1) {
                        m_imCapture.convertTo(m_imCap32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                    }
                    if (m_NeedCV_8UC3) {
                        m_imCapture.convertTo(m_imCap8UC3, CV_8UC3);
                    }
                    break;
                case CV_16UC1:
                    if (m_NeedCV_8UC1) {
                        m_imCapture.convertTo(m_imCap8UC1, CV_8UC1, 1.0 / 256);
                    }
                    if (m_NeedCV_16UC1) {
                        m_imCap16UC1 = m_imCapture;
                    }
                    if (m_NeedCV_32FC1) {
                        m_imCapture.convertTo(m_imCap32FC1, CV_32FC1);  
                    }
                    if (m_NeedCV_8UC3) {
                        m_imCapture.convertTo(m_imCap8UC3, CV_8UC3, 1.0/256);
                    }
                    break;
                case CV_8UC3:
                    if (m_NeedCV_8UC1 || m_NeedCV_16UC1 || m_NeedCV_32FC1) {
                        cv::cvtColor(m_imCapture, m_imCap8UC1, COLOR_RGB2GRAY);
                    }
                    if (m_NeedCV_16UC1) {
                        m_imCap8UC1.convertTo(m_imCap16UC1, CV_16UC1, 256.0);
                    }
                    if (m_NeedCV_32FC1) {
                        m_imCap8UC1.convertTo(m_imCap32FC1, CV_32FC1); // Hmm scale up here?
                    }
                    if (m_NeedCV_8UC3) {
                        m_imCap8UC3 = m_imCapture;
                    }
                    break;
                default:
                    m_Logger->error("Source format unsupported");
                    break;
                }

                // And copy to Out Mats
                // ??? should just be reference to Cap Mats

                if (m_NeedCV_8UC1) {
                    m_imCap8UC1.copyTo(m_imOut8UC1);
                }
                if (m_NeedCV_16UC1) {
                    m_imCap16UC1.copyTo(m_imOut16UC1);
                }
                if (m_NeedCV_32FC1) {
                    m_imCap32FC1.copyTo(m_imOut32FC1);
                }
                if (m_NeedCV_8UC3) {
                    m_imCap8UC3.copyTo(m_imOut8UC3);
                }
            }

        }



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
