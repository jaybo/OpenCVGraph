#pragma once

#include "stdafx.h"

using namespace std;
using namespace spdlog;

namespace openCVGraph
{

    const int FROM_YAML = -1;   // For Tri-state [TRUE=1, FALSE=0, YAML=-1]

    // Which data stream should each filter process

    enum StreamIn {
        CaptureRaw = 0,
        CaptureProcessed = 1,
        Out = 2
    };

    // --------------------------------------------------
    // Container for data which is shared by ALL graphs
    // --------------------------------------------------

    class GraphCommonData {
    public:
        // Keep track of what formats are available, 
        // so the conversion only happens once.

        bool m_Have_CV_8UC1 = false;
        bool m_Have_CV_8UC3 = false;
        bool m_Have_CV_16UC1 = false;
        bool m_Have_CV_32FC1 = false;

        bool m_HaveGpu_CV_8UC1 = false;
        bool m_HaveGpu_CV_8UC3 = false;
        bool m_HaveGpu_CV_16UC1 = false;
        bool m_HaveGpu_CV_32FC1 = false;

        cv::Mat m_imCapture;                // Raw Capture image.  Always keep this unmodified
        cv::Mat m_imCaptureCorrected;       // Capture after Bright/Dark, Spatial, upshift

        cv::Mat m_imCap8UC3;                // Alternate formats of the Capture->CaptureCorrected image
        cv::Mat m_imCap8UC1;
        cv::Mat m_imCap16UC1;
        cv::Mat m_imCap32FC1;

        cv::Mat m_imPreview;                // downsampled and 8bpp
        int m_PreviewDownsampleFactor = 0;  // 0 disables filling m_imPreview;

#ifdef WITH_CUDA
        cv::cuda::GpuMat m_imCaptureGpu;            // Raw Capture image.  Always keep this unmodified
        cv::cuda::GpuMat m_imCaptureCorrectedGpu;   // Capture after Bright/Dark, Spatial, upshift

        cv::cuda::GpuMat m_imCapGpu8UC3;            // Alternate formats of the Capture->CaptureCorrected image on Gpu
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

        //cv::Mat m_imOut;

        // Cuda!
        bool m_UseCuda = true;

#ifdef WITH_CUDA
        // Cuda output Mats
        //cv::cuda::GpuMat m_imOutGpu;
        cv::cuda::GpuMat m_imOutGpu8UC3;
        cv::cuda::GpuMat m_imOutGpu8UC1;
        cv::cuda::GpuMat m_imOutGpu16UC1;
        cv::cuda::GpuMat m_imOutGpu32FC1;
#endif

        GraphCommonData * m_CommonData;
        std::shared_ptr<logger> m_Logger;
        int m_FrameNumber = 0;                  // Current frame being processed.

        // A capture or source filter has already put an image into m_CommonData->m_imCapture.
        // Upload a copy to Cuda.
        // A basic rule is that m_CommonData->m_imCapture should never be modified by any filter downstream, so it's always available.
        void UploadCaptureToCuda()
        {
            ResetImageCache();                  // New image, so clear the available flags

            if (m_UseCuda) {
#ifdef WITH_CUDA
                m_CommonData->m_imCaptureGpu.upload(m_CommonData->m_imCapture);
#endif
            }
        }

        // At the start of the loop, mark all buffers invalid
        void ResetImageCache()
        {
            m_CommonData->m_Have_CV_8UC1 = false;
            m_CommonData->m_Have_CV_8UC3 = false;
            m_CommonData->m_Have_CV_16UC1 = false;
            m_CommonData->m_Have_CV_32FC1 = false;

            m_CommonData->m_HaveGpu_CV_8UC1 = false;
            m_CommonData->m_HaveGpu_CV_8UC3 = false;
            m_CommonData->m_HaveGpu_CV_16UC1 = false;
            m_CommonData->m_HaveGpu_CV_32FC1 = false;
        }

        // To avoid unnecesary copies of the image and reallocation of buffers,
        // this routine copies the capture buffer to the requested format
        // and prevents duplicate copies from happening.

        void EnsureFormatIsAvailable(bool cuda, int needFormat)
        {
            int nChannels = m_CommonData->m_imCapture.channels();
            int nDepth = m_CommonData->m_imCapture.depth();
            int nType = m_CommonData->m_imCapture.type();

            if (cuda) {
#ifdef WITH_CUDA
                switch (nType) {  // original capture format

                case CV_8UC1:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (!m_CommonData->m_HaveGpu_CV_8UC1) {
                            m_CommonData->m_imCaptureGpu.copyTo(m_CommonData->m_imCapGpu8UC1);
                            m_CommonData->m_HaveGpu_CV_8UC1 = true;
                        }
                        break;
                    case CV_16UC1:
                        if (!m_CommonData->m_HaveGpu_CV_16UC1) {
                            m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu16UC1, CV_16UC1, 256.0);
                            m_CommonData->m_HaveGpu_CV_16UC1 = true;
                        }
                        break;
                    case CV_32FC1:
                        if (!m_CommonData->m_HaveGpu_CV_32FC1) {
                            m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                            m_CommonData->m_HaveGpu_CV_32FC1 = true;
                        }
                        break;
                    case CV_8UC3:
                        if (!m_CommonData->m_HaveGpu_CV_8UC3) {
                            cuda::cvtColor(m_CommonData->m_imCapGpu8UC1, m_CommonData->m_imCapGpu8UC3, COLOR_RGB2GRAY);
                            //m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu8UC3, CV_8UC3);
                            m_CommonData->m_HaveGpu_CV_8UC3 = true;
                        }
                        break;
                    default:
                        m_Logger->error("Destination format unsupported");
                        break;
                    }
                    break;

                case CV_16UC1:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (!m_CommonData->m_HaveGpu_CV_8UC1) {
                            m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu8UC1, CV_8UC1, 1.0 / 256);
                            m_CommonData->m_HaveGpu_CV_8UC1 = true;
                        }
                        break;
                    case CV_16UC1:
                        if (!m_CommonData->m_HaveGpu_CV_16UC1) {
                            m_CommonData->m_imCaptureGpu.copyTo(m_CommonData->m_imCapGpu16UC1);
                            m_CommonData->m_HaveGpu_CV_16UC1 = true;
                        }
                        break;
                    case CV_32FC1:
                        if (!m_CommonData->m_HaveGpu_CV_32FC1) {
                            m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCapGpu32FC1, CV_32FC1);
                            m_CommonData->m_HaveGpu_CV_32FC1 = true;
                        }
                        break;
                    case CV_8UC3:
                        if (!m_CommonData->m_HaveGpu_CV_8UC3) {
                            EnsureFormatIsAvailable(cuda, CV_8UC1);
                            cuda::cvtColor(m_CommonData->m_imCapGpu8UC1, m_CommonData->m_imCapGpu8UC3, COLOR_GRAY2RGB);
                            m_CommonData->m_HaveGpu_CV_8UC3 = true;
                        }
                        break;
                    default:
                        m_Logger->error("Destination format unsupported");
                        break;
                    }
                    break;

                case CV_8UC3:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (!m_CommonData->m_HaveGpu_CV_8UC1) {
                            cuda::cvtColor(m_CommonData->m_imCaptureGpu, m_CommonData->m_imCapGpu8UC1, COLOR_RGB2GRAY);
                            m_CommonData->m_HaveGpu_CV_8UC1 = true;
                        }                        
                        break;
                    case CV_16UC1:
                        if (!m_CommonData->m_HaveGpu_CV_16UC1) {
                            EnsureFormatIsAvailable(cuda, CV_8UC1);
                            m_CommonData->m_imCapGpu8UC1.convertTo(m_CommonData->m_imCapGpu16UC1, CV_16UC1, 256.0);
                            m_CommonData->m_HaveGpu_CV_16UC1 = true;
                        }                        
                        break;
                    case CV_32FC1:
                        if (!m_CommonData->m_HaveGpu_CV_32FC1) {
                            EnsureFormatIsAvailable(cuda, CV_8UC1);
                            m_CommonData->m_imCapGpu8UC1.convertTo(m_CommonData->m_imCapGpu32FC1, CV_32FC1); // Hmm scale up here?
                            m_CommonData->m_HaveGpu_CV_32FC1 = true;
                        }
                        break;
                    case CV_8UC3:
                        if (!m_CommonData->m_HaveGpu_CV_8UC3) {
                            m_CommonData->m_imCaptureGpu.copyTo(m_CommonData->m_imCapGpu8UC3);
                            m_CommonData->m_HaveGpu_CV_8UC3 = true;
                        }
                        break;
                    default:
                        m_Logger->error("Destination format unsupported");
                        break;
                    }
                    break;
                default:
                    m_Logger->error("Source format unsupported");
                }
#endif
            }
            else {
                switch (nType) {
                case CV_8UC1:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (!m_CommonData->m_Have_CV_8UC1) {
                            m_CommonData->m_imCap8UC1 = m_CommonData->m_imCapture;
                            m_CommonData->m_Have_CV_8UC1 = true;
                        }
                        break;
                    case CV_16UC1:
                        if (!m_CommonData->m_Have_CV_16UC1) {
                            m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap16UC1, CV_16UC1, 256.0);
                            m_CommonData->m_Have_CV_16UC1 = true;
                        }
                        break;
                    case CV_32FC1:
                        if (!m_CommonData->m_Have_CV_32FC1) {
                            m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                            m_CommonData->m_Have_CV_32FC1 = true;
                        }
                        break;
                    case CV_8UC3:
                        if (!m_CommonData->m_Have_CV_8UC3) {
                            m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap8UC3, CV_8UC3);
                            m_CommonData->m_Have_CV_8UC3 = true;
                        }
                        break;
                    default:
                        m_Logger->error("Destination format unsupported");
                        break;
                    }
                    break;

                case CV_16UC1:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (!m_CommonData->m_Have_CV_8UC1) {
                            m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap8UC1, CV_8UC1, 1.0 / 256);
                            m_CommonData->m_Have_CV_8UC1 = true;
                        }                        
                        break;
                    case CV_16UC1:
                        if (!m_CommonData->m_Have_CV_16UC1) {
                            m_CommonData->m_imCap16UC1 = m_CommonData->m_imCapture;
                            m_CommonData->m_Have_CV_16UC1 = true;
                        }
                        break;
                    case CV_32FC1:
                        if (!m_CommonData->m_Have_CV_32FC1) {
                            m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap32FC1, CV_32FC1);
                            m_CommonData->m_Have_CV_32FC1 = true;
                        }
                        break;
                    case CV_8UC3:
                        if (!m_CommonData->m_Have_CV_8UC3) {
                            m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCap8UC3, CV_8UC3, 1.0 / 256);
                            m_CommonData->m_Have_CV_8UC3 = true;
                        }
                        break;
                    default:
                        m_Logger->error("Destination format unsupported");
                        break;
                    }
                    break;

                case CV_8UC3:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (!m_CommonData->m_Have_CV_8UC1) {
                            cv::cvtColor(m_CommonData->m_imCapture, m_CommonData->m_imCap8UC1, COLOR_RGB2GRAY);
                            m_CommonData->m_Have_CV_8UC1 = true;
                        }
                        break;
                    case CV_16UC1:
                        if (!m_CommonData->m_Have_CV_16UC1) {
                            EnsureFormatIsAvailable(cuda, CV_8UC1);
                            m_CommonData->m_imCap8UC1.convertTo(m_CommonData->m_imCap16UC1, CV_16UC1, 256.0);
                            m_CommonData->m_Have_CV_16UC1 = true;
                        }
                        break;
                    case CV_32FC1:
                        if (!m_CommonData->m_Have_CV_32FC1) {
                            EnsureFormatIsAvailable(cuda, CV_8UC1);
                            m_CommonData->m_imCap8UC1.convertTo(m_CommonData->m_imCap32FC1, CV_32FC1); // Hmm scale up here?
                            m_CommonData->m_Have_CV_32FC1 = true;
                        }
                        break;
                    case CV_8UC3:
                        if (!m_CommonData->m_Have_CV_8UC3) {
                            m_CommonData->m_imCap8UC3 = m_CommonData->m_imCapture;
                            m_CommonData->m_Have_CV_8UC3 = true;
                        }
                        break;
                    default:
                        m_Logger->error("Destination format unsupported");
                        break;
                    }
                    break;
                default:
                    m_Logger->error("Source format unsupported");
                    break;
                }
            }
        }
	};
}
