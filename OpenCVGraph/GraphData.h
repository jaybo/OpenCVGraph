#pragma once

#include "stdafx.h"

using namespace std;
using namespace spdlog;

namespace openCVGraph
{
    const int FROM_YAML = -1;   // For Tri-state [TRUE=1, FALSE=0, YAML=-1]

    // Which data stream should each filter process?
    enum StreamIn {
        CaptureRaw = 0,
        Corrected = 1,
        Out = 2
    };

    // --------------------------------------------------
    // Container for data which is shared by ALL graphs
    // --------------------------------------------------

    class GraphCommonData {
    public:
        // Keep track of what formats are available, 
        // so the conversion only happens once.

        bool m_Have_Capture_CV_8UC1 = false;
        bool m_Have_Capture_CV_8UC3 = false;
        bool m_Have_Capture_CV_16UC1 = false;
        bool m_Have_Capture_CV_32FC1 = false;

        bool m_Have_Capture_Gpu_CV_8UC1 = false;
        bool m_Have_Capture_Gpu_CV_8UC3 = false;
        bool m_Have_Capture_Gpu_CV_16UC1 = false;
        bool m_Have_Capture_Gpu_CV_32FC1 = false;

        bool m_Have_Corrected = false;
        bool m_Have_Corrected_CV_8UC1 = false;
        bool m_Have_Corrected_CV_8UC3 = false;
        bool m_Have_Corrected_CV_16UC1 = false;
        bool m_Have_Corrected_CV_32FC1 = false;

        bool m_Have_Corrected_Gpu_CV_8UC1 = false;
        bool m_Have_Corrected_Gpu_CV_8UC3 = false;
        bool m_Have_Corrected_Gpu_CV_16UC1 = false;
        bool m_Have_Corrected_Gpu_CV_32FC1 = false;

        cv::Mat m_imCapture;                        // Raw Capture image.  Always keep this unmodified
        cv::Mat m_imCorrected;                      // Capture after upshift, Bright/Dark, Spatial

        cv::Mat m_imCapture8UC3;                    // Alternate formats of the raw capture image
        cv::Mat m_imCapture8UC1;
        cv::Mat m_imCapture16UC1;
        cv::Mat m_imCapture32FC1;

        cv::Mat m_imCorrected8UC3;                  // Alternate formats of the Capture->CaptureCorrected image
        cv::Mat m_imCorrected8UC1;
        cv::Mat m_imCorrected16UC1;
        cv::Mat m_imCorrected32FC1;

        cv::Mat m_imPreview;                        // downsampled and 8bpp
        int m_PreviewDownsampleFactor = 0;          // 0 disables filling m_imPreview;

#ifdef WITH_CUDA
        cv::cuda::GpuMat m_imCaptureGpu;            // Raw Capture image.  Always keep this unmodified
        cv::cuda::GpuMat m_imCorrectedGpu;          // Capture after upshift, Bright/Dark, Spatial

        cv::cuda::GpuMat m_imCaptureGpu8UC3;        // Alternate formats of the raw capture image on Gpu
        cv::cuda::GpuMat m_imCaptureGpu8UC1;
        cv::cuda::GpuMat m_imCaptureGpu16UC1;
        cv::cuda::GpuMat m_imCaptureGpu32FC1;

        cv::cuda::GpuMat m_imCorrectedGpu8UC3;      // Alternate formats of the Capture->CaptureCorrected image on Gpu
        cv::cuda::GpuMat m_imCorrectedGpu8UC1;
        cv::cuda::GpuMat m_imCorrectedGpu16UC1;
        cv::cuda::GpuMat m_imCorrectedGpu32FC1;
#endif

        int m_ROISizeX = 0;                     // Overall dimensions of ROI grid
        int m_ROISizeY = 0;
        int m_roiX = 0;                         // Current position in ROI grid
        int m_roiY = 0;                             
        string m_SourceFileName;                // Used when source images come from file, directory, or movie
        string m_DestinationFileName;           // Name of output file

        int m_FrameNumber = 0;
        //std::shared_ptr<logger> m_Logger;
    };


    // -----------------------------------------------------------------------------------------------------------------
    // Container for all mats and data which flows through the graph.  
    // Each graph has its own GraphData (with unique Mat headers) but actual Mat data is often shared between graphs.
    // -----------------------------------------------------------------------------------------------------------------

	class  GraphData {
    private:
        std::mutex m_mutex;
    public:
		std::string m_GraphName;			// Name of the loop processor running this graph
		bool m_AbortOnESC;                  // Exit the graph thread if ESC is hit?
        bool m_Aborting = false;

        // Cuda!
        bool m_UseCuda = true;

        // Output Mat
        cv::Mat m_imOut;

#ifdef WITH_CUDA
        // CUDA output Mat
        cv::cuda::GpuMat m_imOutGpu;
#endif

        GraphCommonData * m_CommonData;
        std::shared_ptr<logger> m_Logger;
        int m_FrameNumber = 0;                  // Current frame being processed by this graph.

        // A capture or source filter has already put an image into m_CommonData->m_imCapture.
        // Clear the image cache flags and upload a copy to Cuda if available.
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


        void EnsureCorrectedOnCpu()
        {
            if (m_UseCuda && !m_CommonData->m_Have_Corrected) {
#ifdef WITH_CUDA
                m_CommonData->m_imCorrectedGpu.download(m_CommonData->m_imCorrected);
#endif
            }
            // for non-CUDA scenarios, the CapturePostProcessing filter always is working 
            // with Corrected so no need to copy
            m_CommonData->m_Have_Corrected = true;
        }


        // At the start of the loop, mark all buffers invalid
        void ResetImageCache()
        {
            m_CommonData->m_Have_Capture_CV_8UC1 = false;
            m_CommonData->m_Have_Capture_CV_8UC3 = false;
            m_CommonData->m_Have_Capture_CV_16UC1 = false;
            m_CommonData->m_Have_Capture_CV_32FC1 = false;

            m_CommonData->m_Have_Capture_Gpu_CV_8UC1 = false;
            m_CommonData->m_Have_Capture_Gpu_CV_8UC3 = false;
            m_CommonData->m_Have_Capture_Gpu_CV_16UC1 = false;
            m_CommonData->m_Have_Capture_Gpu_CV_32FC1 = false;

            m_CommonData->m_Have_Corrected = false;
            m_CommonData->m_Have_Corrected_CV_8UC1 = false;
            m_CommonData->m_Have_Corrected_CV_8UC3 = false;
            m_CommonData->m_Have_Corrected_CV_16UC1 = false;
            m_CommonData->m_Have_Corrected_CV_32FC1 = false;

            m_CommonData->m_Have_Corrected_Gpu_CV_8UC1 = false;
            m_CommonData->m_Have_Corrected_Gpu_CV_8UC3 = false;
            m_CommonData->m_Have_Corrected_Gpu_CV_16UC1 = false;
            m_CommonData->m_Have_Corrected_Gpu_CV_32FC1 = false;
        }

        // To avoid unnecesary copies of the image and reallocation of buffers,
        // this routine converts the capture or corrected image to the requested format.
        // 
        // cuda = true means work on the CUDA stream
        // corrected = true means work on the corrected stream

        void EnsureFormatIsAvailable(bool cuda, int needFormat, bool corrected)
        {
            // std::lock_guard<std::mutex> lock(m_mutex);

            int nChannels = m_CommonData->m_imCapture.channels();
            int nDepth = m_CommonData->m_imCapture.depth();
            int nType = m_CommonData->m_imCapture.type();

            if (cuda) {
#ifdef WITH_CUDA
                switch (nType) {  // original capture format

                case CV_8UC1:
                    switch (needFormat) {
                    case CV_8UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_8UC1) {
                                m_CommonData->m_imCorrectedGpu.copyTo(m_CommonData->m_imCorrectedGpu8UC1);
                                m_CommonData->m_Have_Corrected_Gpu_CV_8UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_8UC1) {
                                m_CommonData->m_imCaptureGpu.copyTo(m_CommonData->m_imCaptureGpu8UC1);
                                m_CommonData->m_Have_Capture_Gpu_CV_8UC1 = true;
                            }
                        }
                        break;
                    case CV_16UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_16UC1) {
                                m_CommonData->m_imCorrectedGpu.convertTo(m_CommonData->m_imCorrectedGpu16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Corrected_Gpu_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_16UC1) {
                                m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCaptureGpu16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Capture_Gpu_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_32FC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_16UC1) {
                                m_CommonData->m_imCorrectedGpu.convertTo(m_CommonData->m_imCorrectedGpu16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Corrected_Gpu_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_16UC1) {
                                m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCaptureGpu16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Capture_Gpu_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_8UC3:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_8UC3) {
                                cuda::cvtColor(m_CommonData->m_imCorrectedGpu8UC1, m_CommonData->m_imCorrectedGpu8UC3, COLOR_RGB2GRAY);
                                //m_CommonData->m_imCorrectedGpu.convertTo(m_CommonData->m_imCorrectedGpu8UC3, CV_8UC3);
                                m_CommonData->m_Have_Corrected_Gpu_CV_8UC3 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_8UC3) {
                                cuda::cvtColor(m_CommonData->m_imCaptureGpu8UC1, m_CommonData->m_imCaptureGpu8UC3, COLOR_RGB2GRAY);
                                //m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCaptureGpu8UC3, CV_8UC3);
                                m_CommonData->m_Have_Capture_Gpu_CV_8UC3 = true;
                            }
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
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_8UC1) {
                                m_CommonData->m_imCorrectedGpu.convertTo(m_CommonData->m_imCorrectedGpu8UC1, CV_8UC1, 1.0 / 256);
                                m_CommonData->m_Have_Corrected_Gpu_CV_8UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_8UC1) {
                                m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCaptureGpu8UC1, CV_8UC1, 1.0 / 256);
                                m_CommonData->m_Have_Capture_Gpu_CV_8UC1 = true;
                            }
                        }
                        break;
                    case CV_16UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_16UC1) {
                                m_CommonData->m_imCorrectedGpu.copyTo(m_CommonData->m_imCorrectedGpu16UC1);
                                m_CommonData->m_Have_Corrected_Gpu_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_16UC1) {
                                m_CommonData->m_imCaptureGpu.copyTo(m_CommonData->m_imCaptureGpu16UC1);
                                m_CommonData->m_Have_Capture_Gpu_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_32FC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_32FC1) {
                                m_CommonData->m_imCorrectedGpu.convertTo(m_CommonData->m_imCorrectedGpu32FC1, CV_32FC1);
                                m_CommonData->m_Have_Corrected_Gpu_CV_32FC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_32FC1) {
                                m_CommonData->m_imCaptureGpu.convertTo(m_CommonData->m_imCaptureGpu32FC1, CV_32FC1);
                                m_CommonData->m_Have_Capture_Gpu_CV_32FC1 = true;
                            }
                        }
                        break;
                    case CV_8UC3:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_8UC3) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                cuda::cvtColor(m_CommonData->m_imCorrectedGpu8UC1, m_CommonData->m_imCorrectedGpu8UC3, COLOR_GRAY2RGB);
                                m_CommonData->m_Have_Corrected_Gpu_CV_8UC3 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_8UC3) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                cuda::cvtColor(m_CommonData->m_imCaptureGpu8UC1, m_CommonData->m_imCaptureGpu8UC3, COLOR_GRAY2RGB);
                                m_CommonData->m_Have_Capture_Gpu_CV_8UC3 = true;
                            }
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
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_8UC1) {
                                cuda::cvtColor(m_CommonData->m_imCorrectedGpu, m_CommonData->m_imCorrectedGpu8UC1, COLOR_RGB2GRAY);
                                m_CommonData->m_Have_Corrected_Gpu_CV_8UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_8UC1) {
                                cuda::cvtColor(m_CommonData->m_imCaptureGpu, m_CommonData->m_imCaptureGpu8UC1, COLOR_RGB2GRAY);
                                m_CommonData->m_Have_Capture_Gpu_CV_8UC1 = true;
                            }
                        }
                        break;
                    case CV_16UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_16UC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCorrectedGpu8UC1.convertTo(m_CommonData->m_imCorrectedGpu16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Corrected_Gpu_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_16UC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCaptureGpu8UC1.convertTo(m_CommonData->m_imCaptureGpu16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Capture_Gpu_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_32FC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_32FC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCorrectedGpu8UC1.convertTo(m_CommonData->m_imCorrectedGpu32FC1, CV_32FC1); // Hmm scale up here?
                                m_CommonData->m_Have_Corrected_Gpu_CV_32FC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_32FC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCaptureGpu8UC1.convertTo(m_CommonData->m_imCaptureGpu32FC1, CV_32FC1); // Hmm scale up here?
                                m_CommonData->m_Have_Capture_Gpu_CV_32FC1 = true;
                            }
                        }
                        break;
                    case CV_8UC3:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_Gpu_CV_8UC3) {
                                m_CommonData->m_imCorrectedGpu.copyTo(m_CommonData->m_imCorrectedGpu8UC3);
                                m_CommonData->m_Have_Corrected_Gpu_CV_8UC3 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_Gpu_CV_8UC3) {
                                m_CommonData->m_imCaptureGpu.copyTo(m_CommonData->m_imCaptureGpu8UC3);
                                m_CommonData->m_Have_Capture_Gpu_CV_8UC3 = true;
                            }
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
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_8UC1) {
                                m_CommonData->m_imCorrected8UC1 = m_CommonData->m_imCorrected;
                                m_CommonData->m_Have_Corrected_CV_8UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_8UC1) {
                                m_CommonData->m_imCapture8UC1 = m_CommonData->m_imCapture;
                                m_CommonData->m_Have_Capture_CV_8UC1 = true;
                            }
                        }
                        break;
                    case CV_16UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_16UC1) {
                                m_CommonData->m_imCorrected.convertTo(m_CommonData->m_imCorrected16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Corrected_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_16UC1) {
                                m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCapture16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Capture_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_32FC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_32FC1) {
                                m_CommonData->m_imCorrected.convertTo(m_CommonData->m_imCorrected32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                                m_CommonData->m_Have_Corrected_CV_32FC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_32FC1) {
                                m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCapture32FC1, CV_32FC1, 256.0);  // Hmm always scale up here to fake 16bpp?
                                m_CommonData->m_Have_Capture_CV_32FC1 = true;
                            }
                        }
                        break;
                    case CV_8UC3:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_8UC3) {
                                cv::cvtColor(m_CommonData->m_imCorrected, m_CommonData->m_imCorrected8UC3, CV_8UC3);
                                m_CommonData->m_Have_Corrected_CV_8UC3 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_8UC3) {
                                m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCapture8UC3, CV_8UC3);
                                m_CommonData->m_Have_Capture_CV_8UC3 = true;
                            }
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
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_8UC1) {
                                EnsureCorrectedOnCpu();
                                m_CommonData->m_imCorrected.convertTo(m_CommonData->m_imCorrected8UC1, CV_8UC1, 1.0 / 256);
                                m_CommonData->m_Have_Corrected_CV_8UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_8UC1) {
                                m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCapture8UC1, CV_8UC1, 1.0 / 256);
                                m_CommonData->m_Have_Capture_CV_8UC1 = true;
                            }
                        }
                        break;
                    case CV_16UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_16UC1) {
                                m_CommonData->m_imCorrected16UC1 = m_CommonData->m_imCorrected;
                                m_CommonData->m_Have_Corrected_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_16UC1) {
                                m_CommonData->m_imCapture16UC1 = m_CommonData->m_imCapture;
                                m_CommonData->m_Have_Capture_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_32FC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_32FC1) {
                                m_CommonData->m_imCorrected.convertTo(m_CommonData->m_imCorrected32FC1, CV_32FC1);
                                m_CommonData->m_Have_Corrected_CV_32FC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_32FC1) {
                                m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCapture32FC1, CV_32FC1);
                                m_CommonData->m_Have_Capture_CV_32FC1 = true;
                            }
                        }
                        break;
                    case CV_8UC3:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_8UC3) {
                                m_CommonData->m_imCorrected.convertTo(m_CommonData->m_imCorrected8UC3, CV_8UC3, 1.0 / 256);
                                m_CommonData->m_Have_Corrected_CV_8UC3 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_8UC3) {
                                m_CommonData->m_imCapture.convertTo(m_CommonData->m_imCapture8UC3, CV_8UC3, 1.0 / 256);
                                m_CommonData->m_Have_Capture_CV_8UC3 = true;
                            }
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
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_8UC1) {
                                cv::cvtColor(m_CommonData->m_imCorrected, m_CommonData->m_imCorrected8UC1, COLOR_RGB2GRAY);
                                m_CommonData->m_Have_Corrected_CV_8UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_8UC1) {
                                cv::cvtColor(m_CommonData->m_imCapture, m_CommonData->m_imCapture8UC1, COLOR_RGB2GRAY);
                                m_CommonData->m_Have_Capture_CV_8UC1 = true;
                            }
                        }
                        break;
                    case CV_16UC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_16UC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCorrected8UC1.convertTo(m_CommonData->m_imCorrected16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Corrected_CV_16UC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_16UC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCapture8UC1.convertTo(m_CommonData->m_imCapture16UC1, CV_16UC1, 256.0);
                                m_CommonData->m_Have_Capture_CV_16UC1 = true;
                            }
                        }
                        break;
                    case CV_32FC1:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_32FC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCorrected8UC1.convertTo(m_CommonData->m_imCorrected32FC1, CV_32FC1); // Hmm scale up here?
                                m_CommonData->m_Have_Corrected_CV_32FC1 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_32FC1) {
                                EnsureFormatIsAvailable(cuda, CV_8UC1, corrected);
                                m_CommonData->m_imCapture8UC1.convertTo(m_CommonData->m_imCapture32FC1, CV_32FC1); // Hmm scale up here?
                                m_CommonData->m_Have_Capture_CV_32FC1 = true;
                            }
                        }
                        break;
                    case CV_8UC3:
                        if (corrected) {
                            if (!m_CommonData->m_Have_Corrected_CV_8UC3) {
                                m_CommonData->m_imCorrected8UC3 = m_CommonData->m_imCorrected;
                                m_CommonData->m_Have_Corrected_CV_8UC3 = true;
                            }
                        }
                        else
                        {
                            if (!m_CommonData->m_Have_Capture_CV_8UC3) {
                                m_CommonData->m_imCapture8UC3 = m_CommonData->m_imCapture;
                                m_CommonData->m_Have_Capture_CV_8UC3 = true;
                            }
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
