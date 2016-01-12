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
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : Filter(name, graphData, sourceFormat, width, height)
        {
        }

        bool CapturePostProcessing::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            if (m_Enabled) {
                // get the Bright Dark images from file

                Mat img = imread(m_BrightFieldPath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_GRAYSCALE);
                if (!img.empty()) {
                    m_imBrightFieldGpu16U.upload(img);
                }
                else {
                    graphData.m_Logger->error("Unable to load BrightField: " + m_BrightFieldPath);
                    return false;
                }

                img = imread(m_DarkFieldPath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_GRAYSCALE);
                if (!img.empty()) {
                    m_imDarkFieldGpu16U.upload(img);
                }
                else {
                    graphData.m_Logger->error("Unable to load DarkField: " + m_DarkFieldPath);
                    return false;
                }

                // Bright - Dark as 32F
                cuda::subtract(m_imBrightFieldGpu16U, m_imDarkFieldGpu16U, m_imTemp16UGpu);
                m_imTemp16UGpu.convertTo(m_imBrightMinusDarkFieldGpu32F, CV_32F);

                if (m_showView) {
                    // To write on the overlay, you must allocate it.
                    // This indicates to the renderer the need to merge it with the final output image.
                    // m_imViewTextOverlay = Mat(m_ViewWidth, m_ViewHeight, CV_8U);

                    if (m_showViewControls) {
                        createTrackbar("Image: ", m_CombinedName, &m_FieldToView, 3);
                    }
                }
            }
            return true;
        }

        ProcessResult CapturePostProcessing::process(GraphData& graphData) override
        {
            if (graphData.m_UseCuda) {
                
                // upshift
                if (m_CorrectionUpshift4Bits) {
                    cv::cuda::lshift(graphData.m_CommonData->m_imCapGpu16UC1, Scalar(4), graphData.m_CommonData->m_imCapGpu16UC1);
                }

                if (m_CorrectionBrightDark) {
                    // sub darkfield
                    cuda::subtract(graphData.m_CommonData->m_imCapGpu16UC1, m_imDarkFieldGpu16U, m_imTemp16UGpu);
                    // make 32F
                    m_imTemp16UGpu.convertTo(m_imTemp32FGpu, CV_32F);
                    // image - dark / bright - dark
                    cuda::divide(m_imTemp32FGpu, m_imBrightMinusDarkFieldGpu32F, m_imTemp16UGpu);
                }

                if (m_OverwriteCapture) {
                    m_imTemp16UGpu.convertTo(graphData.m_imOutGpu16UC1, CV_16U, 65536.0);
                    graphData.m_imOutGpu16UC1.download(graphData.m_CommonData->m_imCapture);
                }
            }
            else {
                //todo
                assert(false);

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
                    graphData.m_CommonData->m_imCapGpu16UC1.download(m_imView);
                    str << "capture";
                    break;
                case 1:
                    graphData.m_imOutGpu16UC1.download(m_imView);
                    str << "corrected";
                    break;
                case 2:
                    m_imBrightFieldGpu16U.download(m_imView);
                    str << "bright";
                    break;
                case 3:
                    m_imDarkFieldGpu16U.download(m_imView);
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
            fs << "overwrite_capture" << m_OverwriteCapture;
        }

        void  CapturePostProcessing::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["correction_bright_dark"] >> m_CorrectionBrightDark;
            fs["correction_upshift4bits"] >> m_CorrectionUpshift4Bits;
            fs["bright_field_path"] >> m_BrightFieldPath;
            fs["dark_field_path"] >> m_DarkFieldPath;
            fs["overwrite_capture"] >> m_OverwriteCapture;
        }

    private:
        cv::cuda::GpuMat m_imTemp16UGpu;                     // 32F
        cv::cuda::GpuMat m_imTemp32FGpu;                     // 32F
        cv::cuda::GpuMat m_imBrightFieldGpu16U;           // 16U
        cv::cuda::GpuMat m_imDarkFieldGpu16U;             // 16U
        cv::cuda::GpuMat m_imBrightMinusDarkFieldGpu32F;  // 32F

        std::string m_BrightFieldPath = "config/BrightField.tif";
        std::string m_DarkFieldPath = "config/DarkField.tif";

        int m_FieldToView = 0;                      // 0 is processed, 1 is unprocessed, 2 is darkfield, 3 is brightfield
        bool m_CorrectionBrightDark = true;         // perform bright dark field correction
        bool m_CorrectionUpshift4Bits = true;       // multiply incoming 12 bit data by 16 to convert to full range 16bpp
        bool m_OverwriteCapture = true;                // update original capture image
    };
}
