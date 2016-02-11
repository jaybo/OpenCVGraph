#pragma once



#include "..\stdafx.h"
#include "opencv2/xfeatures2d/cuda.hpp"
#include "opencv2/cudafeatures2d.hpp"



using namespace cv;
using namespace cuda;
using namespace std;

namespace openCVGraph
{
    // Matcher:
    //    Match part of reference image with current image


    class Matcher : public Filter
    {
    public:
        Matcher::Matcher(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool Matcher::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            if (m_Enabled) {
                // Init both CUDA and non-CUDA paths

                if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
#endif
                }
                else {
                }

                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("Image: ", m_CombinedName, &m_FieldToView, 3);
                        createTrackbar("Hessian: ", m_CombinedName, &m_HessianThreshold, 4000);
                    }
                }
            }
            return true;
        }

        bool Matcher::processKeyboard(GraphData& data, int key) override
        {
            bool fOK = true;
            if (key == 'N' || key == 'n') {
                m_GrabNewTemplate = true;    // Grab new Image if N key is pressed
            }
            return fOK;
        }

        ProcessResult Matcher::process(GraphData& graphData) override
        {
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, true);

            if (graphData.m_CommonData->m_imCorrectedGpu8UC1.empty()) {
                return ProcessResult::OK;
            }

            // template at center of image
            auto templateROI = Rect(
                graphData.m_CommonData->m_imCapture.size().width / 2 - m_TemplateSizeX / 2,
                graphData.m_CommonData->m_imCapture.size().height / 2 - m_TemplateSizeY / 2,
                m_TemplateSizeX, m_TemplateSizeY);

            if (graphData.m_UseCuda) {

#ifdef WITH_CUDA
                if (m_GrabNewTemplate) {
                    m_GrabNewTemplate = false;
                    graphData.m_CommonData->m_imCorrectedGpu8UC1(templateROI).copyTo(m_imTemplateGpu);
                    m_imTemplateGpu.download(m_imTemplate);
                    m_TemplatePosTopLeft = Point2f(
                        (float)graphData.m_CommonData->m_imCorrectedGpu8UC1.size().width / 2 - m_TemplateSizeX/2, 
                        (float)graphData.m_CommonData->m_imCorrectedGpu8UC1.size().height / 2 - m_TemplateSizeY/2);

                    switch (m_DescriptorAlgo)
                    {
                    case 0:  // Surf
                        m_surf.hessianThreshold = m_HessianThreshold;
                        m_surf.extended = true;
                        m_surf.upright = true;
                        m_ObjectKeypointsGPU.empty();
                        m_ObjectDescriptorsGPU.empty();
                        try {
                            m_surf(m_imTemplateGpu, GpuMat(), m_ObjectKeypointsGPU, m_ObjectDescriptorsGPU);
                        }
                        catch (...) {
                            m_GrabNewTemplate = true;
                            return ProcessResult::OK;
                        }
                        if (m_ObjectDescriptorsGPU.empty()) {
                            m_GrabNewTemplate = true;
                        }
                        break;
                    }

                    switch (m_MatcherAlgo)
                    {
                    case 0:  // Brute force
                        m_matcherBF = cv::cuda::DescriptorMatcher::createBFMatcher(NORM_L2);
                        break;
                    case 1:  // Flann
                        //m_matcherFlann = cv::cuda::DescriptorMatcher::;
                        break;

                    }

                }
                else {
                    switch (m_DescriptorAlgo)
                    {
                    case 0: // Surf
                        try {
                            m_surf(graphData.m_CommonData->m_imCorrectedGpu8UC1, GpuMat(), m_SceneKeypointsGPU, m_SceneDescriptorsGPU);
                        }
                        catch (...) {
                            auto k = 1;
                            return ProcessResult::OK;
                        }
                       if (m_SceneDescriptorsGPU.empty()) {
                           break;
                       }
                       //cout << "FOUND " << m_ObjectKeypointsGPU.cols << " keypoints on first image" << endl;
                       //cout << "FOUND " << m_SceneKeypointsGPU.cols << " keypoints on second image" << endl;

                       // match descriptors
                       m_matcherBF->match(m_ObjectDescriptorsGPU, m_SceneDescriptorsGPU, m_matches);
                       // m_matcherBF->knnMatch(m_ObjectDescriptorsGPU, m_SceneDescriptorsGPU, m_matches2D, 2);

                       // download results

                       m_surf.downloadKeypoints(m_ObjectKeypointsGPU, m_ObjectKeypoints);
                       m_surf.downloadKeypoints(m_SceneKeypointsGPU, m_SceneKeypoints);
                       m_surf.downloadDescriptors(m_ObjectDescriptorsGPU, m_ObjectDescriptors);
                       m_surf.downloadDescriptors(m_SceneDescriptorsGPU, m_SceneDescriptors);

                       double max_dist = 0; double min_dist = 100000;

                       //-- Quick calculation of max and min distances between keypoints
                       for (int i = 0; i < m_matches.size(); i++)
                       {
                           double dist = m_matches[i].distance;
                           if (dist < min_dist) min_dist = dist;
                           if (dist > max_dist) max_dist = dist;
                       }
                       cout << "-- Max dist : " << max_dist << std::endl;
                       cout << "-- Min dist : " << min_dist << std::endl;
                       
                       //-- Collect only "good" matches (i.e. whose distance is less than 3*min_dist )
                       m_good_matches.clear();
                       for (int i = 0; i < m_matches.size(); i++)
                       {
                           if (m_matches[i].distance < 3 * min_dist)
                           {
                               m_good_matches.push_back(m_matches[i]);
                           }
                       }

                       //-- Localize the object
                       m_ObjectPoints.clear();
                       m_ScenePoints.clear();
                       m_good_scene_keypoints.clear();
                       for (size_t i = 0; i < m_good_matches.size(); i++)
                       {
                           //-- Get the keypoints from the good matches
                           m_good_scene_keypoints.push_back(m_SceneKeypoints[m_good_matches[i].trainIdx]);
                           m_ObjectPoints.push_back(m_ObjectKeypoints[m_good_matches[i].queryIdx].pt);
                           m_ScenePoints.push_back(m_SceneKeypoints[m_good_matches[i].trainIdx].pt);
                       }
                       
                       // if using perspective, then...
                       // m_H = findHomography(m_ObjectPoints, m_ScenePoints, RANSAC);

                       m_H = estimateRigidTransform(m_ObjectPoints, m_ScenePoints, false);

                       if (!m_H.empty()) {
                           // Get the corners from the template 
                           std::vector<Point2f> obj_corners(4);
                           obj_corners[0] = cvPoint(0, 0); obj_corners[1] = cvPoint(m_imTemplate.cols, 0);
                           obj_corners[2] = cvPoint(m_imTemplate.cols, m_imTemplate.rows); obj_corners[3] = cvPoint(0, m_imTemplate.rows);

                           m_SceneCorners.resize(4);
                           float dx = (float)m_H.at<double>(0, 2);
                           float dy = (float)m_H.at<double>(1, 2);
                           m_dX = m_TemplatePosTopLeft.x - dx;
                           m_dY = m_TemplatePosTopLeft.y - dy;
                           m_distance = sqrt(m_dX * m_dX + m_dY * m_dY);
                           m_Rotation = atan2f(m_dY, m_dX);
                           for (int i = 0; i < 4; i++) {
                               m_SceneCorners[i] = obj_corners[i] + Point2f(m_dX, m_dY) + m_TemplatePosTopLeft;
                           }
                            // If we were doing perspective xforms we'd use:
                            // perspectiveTransform(obj_corners, m_SceneCorners, m_H);
                       }
                       break;
                    }
                }

#endif
            }
            else {
            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Matcher::processView(GraphData & graphData) override
        {
            if (m_showView) {
                if (m_ObjectKeypoints.empty() || m_SceneKeypoints.empty()) {
                    return;
                }

                graphData.EnsureFormatIsAvailable(false, CV_8UC1, true);                // we always need CPU 8 bit

                ClearOverlayText();

                std::ostringstream str;
                int posLeft = 10;
                double scale = 1.0;
                float units_per_pixel = 1;

                switch (m_FieldToView) {
                case 0: // pixel units displayed
                case 1: // real world units displayed
                    drawKeypoints(graphData.m_CommonData->m_imCorrected8UC1, m_good_scene_keypoints, m_imView, Scalar(255, 0, 0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

                    if (!m_H.empty() && (m_SceneCorners.size() == 4)) {
                        //-- Draw lines between the corners (the mapped object in the m_ScenePoints - image_2 )
                        line(m_imView, m_SceneCorners[0], m_SceneCorners[1], Scalar(0, 255, 0), 8);
                        line(m_imView, m_SceneCorners[1], m_SceneCorners[2], Scalar(0, 255, 0), 8);
                        line(m_imView, m_SceneCorners[2], m_SceneCorners[3], Scalar(0, 255, 0), 8);
                        line(m_imView, m_SceneCorners[3], m_SceneCorners[0], Scalar(0, 255, 0), 8);

                        // draw displacement line
                        Point2f centerObj = Point2f(m_SceneCorners[0].x + m_TemplateSizeX/2, m_SceneCorners[0].y + m_TemplateSizeY/2);
                        Point2f centerScene = graphData.m_CommonData->m_imCapture.size() / 2;
                        line(m_imView, centerObj, centerScene, Scalar(0, 255, 0), 8);

                    }
                    if (m_FieldToView == 0) {
                        str << "matches, pixels";
                    }
                    else {
                        units_per_pixel = m_UnitsPerPixel;
                        str << "matches, " << m_UnitsPerPixel << " " << m_Units << "/pixel";
                    }
                    break;
                case 2:
                    drawMatches(Mat(m_imTemplate), m_ObjectKeypoints,
                        Mat(graphData.m_CommonData->m_imCorrected8UC1),
                        m_SceneKeypoints, m_good_matches,
                        m_imView,
                        Scalar::all(-1), Scalar::all(-1),
                        std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
                    str << "good matches";
                    break;

                case 3:
                    drawKeypoints(m_imTemplate, m_ObjectKeypoints, m_imView, Scalar(255, 0, 0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                    str << "template";
                    break;

                }


                DrawOverlayText(str.str(), Point(posLeft, 30), scale, CV_RGB(128, 128, 128));

                str.str("");
                str << "dX: " << fixed << left << setprecision(2) << setw(14)  << m_dX * units_per_pixel;
                DrawOverlayText(str.str(), Point(posLeft, 300), scale);

                str.str("");
                str << "dY: " << fixed << left << setprecision(2) << setw(14) << m_dY * units_per_pixel;
                DrawOverlayText(str.str(), Point(posLeft, 340), scale);

                str.str("");
                str << "dist: " << fixed << left << setprecision(2) << setw(14) << m_distance * units_per_pixel;
                DrawOverlayText(str.str(), Point(posLeft, 400), scale);

                str.str("");
                auto rotDegrees = RAD2DEG(m_Rotation);
                str << "ang: " << fixed << left << setprecision(2) << setw(14) << ((m_FieldToView == 0) ? m_Rotation : rotDegrees);
                DrawOverlayText(str.str(), Point(posLeft, 440), scale);

                str.str("");
                str << "'N' to grab new template";
                DrawOverlayText(str.str(), Point(posLeft, 500), scale);

                Filter::processView(graphData);
            }
        }

        void  Matcher::saveConfig(FileStorage& fs, GraphData& data)
        {
            Filter::saveConfig(fs, data);
            fs << "template_sizeX" << m_TemplateSizeX;
            fs << "template_sizeY" << m_TemplateSizeY;
            fs << "hessian_threshold" << m_HessianThreshold;
            fs << "units_per_pixel" << m_UnitsPerPixel;
            fs << "units" << m_Units;
        }

        void  Matcher::loadConfig(FileNode& fs, GraphData& data)
        {
            Filter::loadConfig(fs, data);
            // only set these from constructor
            fs["template_sizeX"] >> m_TemplateSizeX;
            fs["template_sizeY"] >> m_TemplateSizeY;
            fs["hessian_threshold"] >> m_HessianThreshold;
            fs["units_per_pixel"] >> m_UnitsPerPixel;
            fs["units"] >> m_Units;
            if (m_UnitsPerPixel == 0) {
                m_UnitsPerPixel = 1;
            }
            if (m_Units == "") {
                m_Units = "nM";
            }
        }

    private:
#ifdef WITH_CUDA
        cv::cuda::GpuMat m_imTemplateGpu;                     

        SURF_CUDA m_surf;
        GpuMat m_ObjectKeypointsGPU, m_SceneKeypointsGPU;
        GpuMat m_ObjectDescriptorsGPU, m_SceneDescriptorsGPU;


#endif
        Mat m_imTemplate;                           // 32F
        Mat m_imTemp32F;  
        float m_dX = 0;
        float m_dY = 0;
        float m_distance = 0;
        float m_Rotation = 0;

        vector<KeyPoint> m_ObjectKeypoints, m_SceneKeypoints;
        vector<float> m_ObjectDescriptors, m_SceneDescriptors;

        std::vector<Point2f> m_ObjectPoints;
        std::vector<Point2f> m_ScenePoints;
        std::vector<Point2f> m_SceneCorners;

        Point2f m_TemplatePosTopLeft;

        vector<DMatch> m_matches;
        std::vector< DMatch > m_good_matches;
        vector<KeyPoint> m_good_scene_keypoints;

        vector<std::vector<cv::DMatch> > m_matches2D;

        Ptr<cv::cuda::DescriptorMatcher> m_matcherBF;
        Ptr<cv::cuda::DescriptorMatcher> m_matcherFlann;
        Ptr<cv::cuda::DescriptorMatcher> m_matcherKNN;

        Mat m_H;                            // homography
        int m_DescriptorAlgo = 0;           // Algo to create descriptors
        int m_MatcherAlgo = 0;              // Algo to perform matches

        bool m_GrabNewTemplate = true;
        int m_FieldToView = 0;                      // 0 is match vectors, 1 is template, 2 is current
        int m_TemplateSizeX = 256;
        int m_TemplateSizeY = 256;
        int m_HessianThreshold = 400;
        float m_UnitsPerPixel = 1;
        std::string m_Units = "nM";

    };
}
