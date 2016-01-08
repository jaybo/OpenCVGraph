#pragma once

#include "stdafx.h"

using namespace std;
using namespace spdlog;

namespace openCVGraph
{
    // --------------------------------------------------
    // Container for data which is shared by ALL graphs
    // --------------------------------------------------

    class GraphCommonData {
    public:
        // Filters the capture drivers what format they need
        bool m_NeedCV_8UC1 = false;
        bool m_NeedCV_8UC3 = false;
        bool m_NeedCV_16UC1 = false;
        bool m_NeedCV_32FC1 = false;

        cv::Mat m_imCapture;                // Raw Capture image.  Always keep this unmodified

        cv::Mat m_imCap8UC3;                // 
        cv::Mat m_imCap8UC1;
        cv::Mat m_imCap16UC1;
        cv::Mat m_imCap32FC1;

#ifdef WITH_CUDA
        cv::cuda::GpuMat m_imCaptureGpu;    // Raw Capture image.  Always keep this unmodified

        // Cuda input Mats
        cv::cuda::GpuMat m_imCapGpu8UC3;
        cv::cuda::GpuMat m_imCapGpu8UC1;
        cv::cuda::GpuMat m_imCapGpu16UC1;
        cv::cuda::GpuMat m_imCapGpu32FC1;

#endif

        int m_ROISizeX = 0;                     // Overall dimensions of ROI grid
        int m_ROISizeY = 0;
        int m_roiX = 0;                         // Current position in ROI grid
        int m_roiY = 0;                             
        string m_SourceFileName;                // Used when source images come from file, directory, or movie
        string m_DestinationFileName;           // Name of output file

        //std::shared_ptr<logger> m_Logger;
    };


    // -----------------------------------------------------------------------------------------------------------------
    // Container for all mats and data which flows through the graph.  
    // Each graph has its own GraphData (with unique Mat headers) but actual Mat data is often shared between graphs.
    // -----------------------------------------------------------------------------------------------------------------

	class  GraphData {
    public:
		std::string m_GraphName;			// Name of the loop processor running this graph
		bool m_AbortOnESC;                  // Exit the graph thread if ESC is hit?
        bool m_Aborting = false;



        // Output Mats
        cv::Mat m_imOut8UC3;              
        cv::Mat m_imOut8UC1;              
        cv::Mat m_imOut16UC1;             
        cv::Mat m_imOut32FC1;             

        // Cuda!
        bool m_UseCuda = true;

#ifdef WITH_CUDA
        // Cuda output Mats
        cv::cuda::GpuMat m_imOutGpu8UC3;
        cv::cuda::GpuMat m_imOutGpu8UC1;
        cv::cuda::GpuMat m_imOutGpu16UC1;
        cv::cuda::GpuMat m_imOutGpu32FC1;
#endif

        GraphCommonData * m_CommonData;
        std::shared_ptr<logger> m_Logger;
        int m_FrameNumber = 0;                  // Current frame being processed.

        // A capture or source filter has already put an image into m_CommonData->m_imCapture.
        // Now convert it into formats needed by the rest of the graph.
        // A basic rule is that m_CommonData->m_imCapture should never be modified by any filter downstream, so it's always available.

        void CopyCaptureToRequiredFormats()
        {
            int nChannels = m_CommonData->m_imCapture.channels();
            int nDepth = m_CommonData->m_imCapture.depth();
            int nType = m_CommonData->m_imCapture.type();

            if (m_UseCuda) {
#ifdef WITH_CUDA
                m_CommonData->m_imCaptureGpu.upload(m_CommonData->m_imCapture);

                switch (nType) {
                case CV_8UC1:
                    if (m_CommonData->m_NeedCV_8UC1) {
                        m_CommonData->m_imCapGpu8UC1 = m_CommonData->m_imCaptureGpu;
                    }
                    if (m_CommonData->m_NeedCV_16UC1) {
                        m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu16UC1, CV_16UC1, 256.0);
                    }
                    if (m_CommonData->m_NeedCV_32FC1) {
                        m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                    }
                    if (m_CommonData->m_NeedCV_8UC3) {
                        m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu8UC3, CV_8UC3);
                    }
                    break;
                case CV_16UC1:
                    if (m_CommonData->m_NeedCV_8UC1) {
                        m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu8UC1, CV_8UC1, 1.0 / 256);
                    }
                    if (m_CommonData->m_NeedCV_16UC1) {
                        m_CommonData->m_imCapGpu16UC1 = m_CommonData->m_imCaptureGpu;
                    }
                    if (m_CommonData->m_NeedCV_32FC1) {
                        m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu32FC1, CV_32FC1);
                    }
                    if (m_CommonData->m_NeedCV_8UC3) {
                        if (!m_CommonData->m_NeedCV_8UC1) {
                            m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu8UC1, CV_8UC1, 1.0 / 256);
                        }
                        cuda::cvtColor(m_CommonData->m_imCapGpu8UC1, m_CommonData->m_imCapGpu8UC3, COLOR_GRAY2RGB);
                    }
                    break;
                case CV_8UC3:
                    if (m_CommonData->m_NeedCV_8UC1 || m_CommonData->m_NeedCV_16UC1 || m_CommonData->m_NeedCV_32FC1) {
                        cuda::cvtColor(m_CommonData->m_imCaptureGpu, m_CommonData->m_imCapGpu8UC1, COLOR_RGB2GRAY);
                    }
                    if (m_CommonData->m_NeedCV_16UC1) {
                        m_CommonData->m_imCapGpu8UC1.convertTo(m_CommonData->m_imCapGpu16UC1, CV_16UC1, 256.0);
                    }
                    if (m_CommonData->m_NeedCV_32FC1) {
                        m_CommonData->m_imCapGpu8UC1.convertTo(m_CommonData->m_imCapGpu32FC1, CV_32FC1); // Hmm scale up here?
                    }
                    if (m_CommonData->m_NeedCV_8UC3) {
                        m_CommonData->m_imCapGpu8UC3 = m_CommonData->m_imCaptureGpu;
                    }
                    break;
                default:
                    m_Logger->error("Source format unsupported");
                }

                // And copy to Out Mats

                if (m_CommonData->m_NeedCV_8UC1) {
                    m_imOutGpu8UC1 = m_CommonData->m_imCapGpu8UC1;
                }
                if (m_CommonData->m_NeedCV_16UC1) {
                    m_imOutGpu16UC1 = m_CommonData->m_imCapGpu16UC1;
                }
                if (m_CommonData->m_NeedCV_32FC1) {
                    m_imOutGpu32FC1 = m_CommonData->m_imCapGpu32FC1;
                }
                if (m_CommonData->m_NeedCV_8UC3) {
                    m_imOutGpu8UC3 = m_CommonData->m_imCapGpu8UC3;
                }
#endif
            }
            else {
                switch (nType) {
                case CV_8UC1:
                    if (m_CommonData->m_NeedCV_8UC1) {
                        m_CommonData->m_imCap8UC1 = m_CommonData->m_imCapture;
                    }
                    if (m_CommonData->m_NeedCV_16UC1) {
                        m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap16UC1, CV_16UC1, 256.0);
                    }
                    if (m_CommonData->m_NeedCV_32FC1) {
                        m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                    }
                    if (m_CommonData->m_NeedCV_8UC3) {
                        m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap8UC3, CV_8UC3);
                    }
                    break;
                case CV_16UC1:
                    if (m_CommonData->m_NeedCV_8UC1) {
                        m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap8UC1, CV_8UC1, 1.0 / 256);
                    }
                    if (m_CommonData->m_NeedCV_16UC1) {
                        m_CommonData->m_imCap16UC1 = m_CommonData->m_imCapture;
                    }
                    if (m_CommonData->m_NeedCV_32FC1) {
                        m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap32FC1, CV_32FC1);  
                    }
                    if (m_CommonData->m_NeedCV_8UC3) {
                        m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap8UC3, CV_8UC3, 1.0/256);
                    }
                    break;
                case CV_8UC3:
                    if (m_CommonData->m_NeedCV_8UC1 || m_CommonData->m_NeedCV_16UC1 || m_CommonData->m_NeedCV_32FC1) {
                        cv::cvtColor(m_CommonData->m_imCapture, m_CommonData->m_imCap8UC1, COLOR_RGB2GRAY);
                    }
                    if (m_CommonData->m_NeedCV_16UC1) {
                        m_CommonData->m_imCap8UC1.convertTo(m_CommonData->m_imCap16UC1, CV_16UC1, 256.0);
                    }
                    if (m_CommonData->m_NeedCV_32FC1) {
                        m_CommonData->m_imCap8UC1.convertTo(m_CommonData->m_imCap32FC1, CV_32FC1); // Hmm scale up here?
                    }
                    if (m_CommonData->m_NeedCV_8UC3) {
                        m_CommonData->m_imCap8UC3 = m_CommonData->m_imCapture;
                    }
                    break;
                default:
                    m_Logger->error("Source format unsupported");
                    break;
                }

                // And copy to Out Mats

                if (m_CommonData->m_NeedCV_8UC1) {
                    m_imOut8UC1 = m_CommonData->m_imCap8UC1;
                }
                if (m_CommonData->m_NeedCV_16UC1) {
                    m_imOut16UC1 = m_CommonData->m_imCap16UC1;
                }
                if (m_CommonData->m_NeedCV_32FC1) {
                    m_imOut32FC1 = m_CommonData->m_imCap32FC1;
                }
                if (m_CommonData->m_NeedCV_8UC3) {
                    m_imOut8UC3 = m_CommonData->m_imCap8UC3;
                }
            }
        }
	};
}
