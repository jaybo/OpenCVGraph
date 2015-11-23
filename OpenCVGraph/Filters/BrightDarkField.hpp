#pragma once

#ifndef INCLUDE_OCVG_DARKLIGHT
#define INCLUDE_OCVG_DARKLIGHT

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Brightfield / Darkfield correction
    // Corrected = (Image - Darkfield) / (Brightfield - Darkfield) * 2**16 (for 16 bit data)

    class BrightDarkFieldCorrection : public Filter
    {
    public:

        static void BrightDarkFieldCorrection::SliderCallback(int pos, void * userData) {
            BrightDarkFieldCorrection* filter = (BrightDarkFieldCorrection *)userData;
            filter->FieldToView(pos);
        }

        BrightDarkFieldCorrection::BrightDarkFieldCorrection(std::string name, GraphData& graphData,
            int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {

        }

        bool BrightDarkFieldCorrection::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            graphData.m_NeedCV_16UC1 = true;
            // Need 8 bit if we're viewing it
            if (m_showView) {
                graphData.m_NeedCV_8UC1 = true;
            }

            // get the Bright Dark images from file

            Mat img = imread(m_BrightFieldPath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_GRAYSCALE);
            if (!img.empty()) {
                m_imBrightFieldGpu16U.upload(img);
            }
            else {
                assert(false);
            }

            img = imread(m_DarkFieldPath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_GRAYSCALE);
            if (!img.empty()) {
                m_imDarkFieldGpu16U.upload(img);
            }
            else {
                assert(false);
            }

            // Bright - Dark as 32F
            cuda::subtract(m_imBrightFieldGpu16U, m_imDarkFieldGpu16U, m_imBrightMinusDarkFieldGpu32F);
            m_imBrightMinusDarkFieldGpu32F.convertTo(m_imBrightMinusDarkFieldGpu32F, CV_32F);

            if (m_showView) {
                // To write on the overlay, you must allocate it.
                // This indicates to the renderer the need to merge it with the final output image.
                // m_imViewTextOverlay = Mat(m_width, m_height, CV_8U);

                if (m_showSlider) {
                    createTrackbar("BrightDarkFieldCorrection", m_CombinedName, &m_FieldToView, 4, SliderCallback, this);
                }
            }

            return true;
        }

        ProcessResult BrightDarkFieldCorrection::process(GraphData& graphData) override
        {
            Mat t;
            if (graphData.m_UseCuda) {
                cuda::subtract(graphData.m_imCapGpu16UC1, m_imDarkFieldGpu16U, m_imTempGpu);
                
                m_imDarkFieldGpu16U.download(t);
                m_imTempGpu.download(t);

                m_imTempGpu.convertTo(m_imTempGpu, CV_32F);
                m_imTempGpu.download(t);

                
                cuda::divide(m_imTempGpu, m_imBrightMinusDarkFieldGpu32F, m_imTempGpu);
                m_imTempGpu.download(t);


                m_imTempGpu.convertTo(graphData.m_imOutGpu16UC1, CV_16U);

                graphData.m_imOutGpu16UC1.download(t);

                if (graphData.m_NeedCV_8UC1) {
                    graphData.m_imOutGpu16UC1.convertTo(graphData.m_imOutGpu8UC1, CV_8U, 1.0 / 256);
                }
            }
            else {
                //todo
                assert(false);

            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void BrightDarkFieldCorrection::processView(GraphData & graphData) override
        {
            if (m_showView) {
                graphData.m_imOutGpu8UC1.download(m_imView);
                Filter::processView(graphData);
            }
        }

        void BrightDarkFieldCorrection::FieldToView(int n) {
            m_FieldToView = n;
        }

        void  BrightDarkFieldCorrection::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs <<"bright_field_path" << m_BrightFieldPath.c_str();
            fs <<"dark_field_path" << m_DarkFieldPath.c_str();
            fs << "show_slider" << m_showSlider;
        }

        void  BrightDarkFieldCorrection::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            fs["bright_field_path"] >> m_BrightFieldPath;
            fs["dark_field_path"] >> m_DarkFieldPath;
            fs["show_slider"] >> m_showSlider;
        }

    private:
        cv::cuda::GpuMat m_imTempGpu;                     // 32F
        cv::cuda::GpuMat m_imBrightFieldGpu16U;           // 16U
        cv::cuda::GpuMat m_imDarkFieldGpu16U;             // 16U
        cv::cuda::GpuMat m_imBrightMinusDarkFieldGpu32F;  // 32F

        std::string m_BrightFieldPath = "config/BrightField.tif";
        std::string m_DarkFieldPath = "config/DarkField.tif";

        int m_FieldToView = 0;      // 0 is unprocessed, 1 is processed, 2 is darkfield, 3 is brightfield
        bool m_showSlider = true;

    };
}
#endif