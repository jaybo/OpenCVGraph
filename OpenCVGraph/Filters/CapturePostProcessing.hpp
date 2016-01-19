#pragma once

#include "..\stdafx.h"

using namespace cv;
using namespace cuda;
using namespace std;

namespace openCVGraph
{
    // CapturePostProcessing:
    //    optionally:
    //    1. Upshift by 4 bits to full 16 bit range
    //    2. Perform bright and darkfield corrections
    //    3. Perform lens distortion corrections

    // Brightfield / Darkfield correction
    // Corrected = (Image - Darkfield) / (Brightfield - Darkfield) * 2**16 (for 16 bit data)

    class CapturePostProcessing : public Filter
    {
    public:
        CapturePostProcessing::CapturePostProcessing(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool CapturePostProcessing::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            if (m_Enabled) {
                // Init both CUDA and non-CUDA paths

                // get the Bright Dark images from file
                m_imBrightField16U = imread(m_BrightFieldPath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_GRAYSCALE);
                if (m_imBrightField16U.empty()) {
                    graphData.m_Logger->error("Unable to load BrightField: " + m_BrightFieldPath);
                    return false;
                }

                m_imDarkField16U = imread(m_DarkFieldPath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_GRAYSCALE);
                if (m_imDarkField16U.empty()) {
                    graphData.m_Logger->error("Unable to load DarkField: " + m_DarkFieldPath);
                    return false;
                }
                if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                    m_imBrightFieldGpu16U.upload(m_imBrightField16U);
                    m_imDarkFieldGpu16U.upload(m_imDarkField16U);

                    cuda::subtract(m_imBrightFieldGpu16U, m_imDarkFieldGpu16U, m_imTempGpu);
                    m_imTempGpu.convertTo(m_imBrightMinusDarkFieldGpu32F, CV_32F);
#endif
                }
                else {
                    // Bright - Dark as 32F
                    m_imTemp = m_imBrightField16U - m_imDarkField16U;
                    m_imTemp.convertTo(m_imBrightMinusDarkField32F, CV_32F);
                }

                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("Image: ", m_CombinedName, &m_FieldToView, 3);
                    }
                }
            }
            return true;
        }

        ProcessResult CapturePostProcessing::process(GraphData& graphData) override
        {
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_16UC1, false);

            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                // upshift
                if (m_CorrectionUpshift4Bits) {
                    cv::cuda::lshift(graphData.m_CommonData->m_imCaptureGpu16UC1, Scalar(4), graphData.m_CommonData->m_imCaptureGpu16UC1);
                    graphData.m_CommonData->m_imCaptureGpu16UC1.download(graphData.m_CommonData->m_imCapture);
                }

                if (m_CorrectionBrightDark) {
                    // sub darkfield
                    cuda::subtract(graphData.m_CommonData->m_imCaptureGpu16UC1, m_imDarkFieldGpu16U, m_imTempGpu);
                    // make 32F
                    m_imTempGpu.convertTo(m_imTemp32FGpu, CV_32F);
                    // image - dark / bright - dark
                    cuda::divide(m_imTemp32FGpu, m_imBrightMinusDarkFieldGpu32F, m_imTempGpu);
                    // Create the corrected image
                    m_imTempGpu.convertTo(graphData.m_CommonData->m_imCorrectedGpu, CV_16U, UINT16_MAX);
                    graphData.m_imOutGpu = graphData.m_CommonData->m_imCorrectedGpu;

                    // graphData.m_CommonData->m_imCorrectedGpu.download(graphData.m_CommonData->m_imCorrected); // for FileWriter
                }
                else {
                    // no correction applied, just use Capture stream
                    graphData.m_CommonData->m_imCorrectedGpu = graphData.m_CommonData->m_imCaptureGpu;
                    graphData.m_imOutGpu = graphData.m_CommonData->m_imCaptureGpu16UC1;
                }

                // Create Preview Image
                if (m_DownsampleForJpgFactor != 0) {
                    graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, false);
                    if (m_DownsampleForJpgFactor == 1) {
                        graphData.m_CommonData->m_imCaptureGpu8UC1.download(graphData.m_CommonData->m_imPreview);
                    }
                    else {
                        cv::cuda::resize(graphData.m_CommonData->m_imCorrectedGpu, m_imTempGpu,
                            graphData.m_CommonData->m_imCorrectedGpu.size() / m_DownsampleForJpgFactor, 0, 0, CV_INTER_NN);
                        m_imTempGpu.download(graphData.m_CommonData->m_imPreview);
                    }
                }
#endif
            }
            else {
                // upshift
                if (m_CorrectionUpshift4Bits) {
                    graphData.m_CommonData->m_imCapture *= 16;
                }

                if (m_CorrectionBrightDark) {
                    // sub darkfield
                    m_imTemp =  graphData.m_CommonData->m_imCapture16UC1 - m_imDarkField16U;
                    // make 32F
                    m_imTemp.convertTo(m_imTemp32F, CV_32F);
                    // image - dark / bright - dark
                    m_imTemp = m_imTemp32F / m_imBrightMinusDarkField32F;

                    m_imTemp.convertTo(graphData.m_CommonData->m_imCorrected, CV_16U, UINT16_MAX);
                    graphData.m_imOut = graphData.m_CommonData->m_imCorrected;
                }
                else {
                    graphData.m_CommonData->m_imCorrected = graphData.m_CommonData->m_imCapture;
                    graphData.m_imOut = graphData.m_CommonData->m_imCapture;
                }

                // Create Preview Image
                if (m_DownsampleForJpgFactor != 0) {
                    graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, false);
                    if (m_DownsampleForJpgFactor == 1) {
                        graphData.m_CommonData->m_imPreview = graphData.m_CommonData->m_imCapture8UC1;
                    }
                    else {
                        cv::resize(graphData.m_CommonData->m_imCorrected, graphData.m_CommonData->m_imPreview,
                            graphData.m_CommonData->m_imCorrected.size() / m_DownsampleForJpgFactor, 0, 0, CV_INTER_NN);
                    }
                }
            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void CapturePostProcessing::processView(GraphData & graphData) override
        {
            if (m_showView) {
                ClearOverlayText();

                std::ostringstream str;

                switch (m_FieldToView) {
                case 0:
                    if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                        graphData.m_CommonData->m_imCaptureGpu16UC1.download(m_imView);
#endif
                    }
                    else {
                        m_imView = graphData.m_CommonData->m_imCapture16UC1;
                    }
                    str << "capture";
                    break;
                case 1:
                    if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                        graphData.m_CommonData->m_imCorrectedGpu.download(m_imView);
#endif
                    }
                    else {
                        m_imView = graphData.m_CommonData->m_imCorrected;
                    }
                    str << "corrected";
                    break;
                case 2:
                    if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                        m_imBrightFieldGpu16U.download(m_imView);
#endif
                    }
                    else {
                        m_imView = m_imBrightField16U;
                    }
                    str << "bright";
                    break;
                case 3:
                    if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                        m_imDarkFieldGpu16U.download(m_imView);
#endif
                    }
                    else {
                        m_imView = m_imDarkField16U;
                    }
                    str << "dark";
                    break;

                }
                int posLeft = 10;
                double scale = 1.0;

                DrawOverlayText(str.str(), Point(posLeft, 30), scale, CV_RGB(128, 128, 128));

                Filter::processView(graphData);
            }
        }

        void  CapturePostProcessing::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "correction_bright_dark" << m_CorrectionBrightDark;
            fs << "correction_upshift4bits" << m_CorrectionUpshift4Bits;
            fs << "bright_field_path" << m_BrightFieldPath.c_str();
            fs << "dark_field_path" << m_DarkFieldPath.c_str();
            fs << "downsample_preview_factor" << m_DownsampleForJpgFactor;
        }

        void  CapturePostProcessing::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["correction_bright_dark"] >> m_CorrectionBrightDark;
            fs["correction_upshift4bits"] >> m_CorrectionUpshift4Bits;
            fs["bright_field_path"] >> m_BrightFieldPath;
            fs["dark_field_path"] >> m_DarkFieldPath;
            fs["downsample_preview_factor"] >> m_DownsampleForJpgFactor;
        }

    private:
#ifdef WITH_CUDA
        cv::cuda::GpuMat m_imTempGpu;                     // 32F
        cv::cuda::GpuMat m_imTemp32FGpu;                  // 32F
        cv::cuda::GpuMat m_imBrightFieldGpu16U;           // 16U
        cv::cuda::GpuMat m_imDarkFieldGpu16U;             // 16U
        cv::cuda::GpuMat m_imBrightMinusDarkFieldGpu32F;  // 32F
        cv::cuda::GpuMat m_DownsampleForJpgGpu8U;         // 8U
#endif
        Mat m_imTemp;                           // 32F
        Mat m_imTemp32F;                        // 32F
        Mat m_imBrightField16U;                 // 16U
        Mat m_imDarkField16U;                   // 16U
        Mat m_imBrightMinusDarkField32F;        // 32F
        Mat m_DownsampleForJpg8U;               // 8U

        std::string m_BrightFieldPath = "config/BrightField.tif";
        std::string m_DarkFieldPath = "config/DarkField.tif";

        int m_FieldToView = 0;                      // 0 is processed, 1 is unprocessed, 2 is darkfield, 3 is brightfield
        bool m_CorrectionBrightDark = true;         // perform bright dark field correction
        bool m_CorrectionUpshift4Bits = true;       // multiply incoming 12 bit data by 16 to convert to full range 16bpp
        int m_DownsampleForJpgFactor = 4;
    };
}
