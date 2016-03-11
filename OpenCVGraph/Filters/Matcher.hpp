#pragma once
#include "..\stdafx.h"
#include "opencv2/xfeatures2d/cuda.hpp"
#include "opencv2/cudafeatures2d.hpp"

using namespace cv;
using namespace cuda;
using namespace std;

namespace openCVGraph
{
    struct TemplateInfo {
        bool TemplateValid = false;
        GpuMat KeypointsGpu;
        GpuMat DescriptorsGpu;
        vector<KeyPoint> KeypointsCpu;
        vector<float> DescriptorsCpu;
        vector<KeyPoint> GoodKeypointsCpu;
        cv::Rect SourceRect;
    };

    struct MatchDetails {
        MatcherInfo MatcherInfo;
        //vector<DMatch> Matches;
        vector<DMatch> GoodMatches;
        std::vector<Point2f> DetectedPoints;
        std::vector<Point2f> MatchRect;
    };

    // Matcher:
    //    Match part of reference image with current image


    class Matcher : public Filter , ITemcaMatcher
    {
    public:
        Matcher::Matcher(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {}

        bool Matcher::init(GraphData& graphData) override
        {
            Filter::init(graphData);

            if (m_Enabled) {
                if (!graphData.m_UseCuda) {
                    abort();
                }

                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("Image: ", m_CombinedName, &m_FieldToView, 3);
                        createTrackbar("Hessian: ", m_CombinedName, &m_HessianThreshold, 4000);
                    }
                }

                // non-temca mode by default
                grabMatcherTemplate(3840 / 2 - m_TemplateSizeX / 2, 3840 / 2 - m_TemplateSizeY / 2, m_TemplateSizeX, m_TemplateSizeY);
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

        // for now, only using SURF and BF matcher.  Someday, expand the choices...
        void Matcher::SelectAlgorithms() {
            if (m_HessianThreshold != m_HessianThresholdLast) {
                m_HessianThresholdLast = m_HessianThreshold;

                switch (m_DescriptorAlgo)
                {
                case 0:  // Surf
                    m_surf.hessianThreshold = m_HessianThreshold;
                    m_surf.extended = true;
                    m_surf.upright = true;
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
        }


        std::vector<TemplateInfo> Matcher::FindDescriptorsAndKeypoints(GraphData& graphData)
        {
            std::vector<TemplateInfo> templates;

            for (auto roi : m_ROIs) {
                TemplateInfo* ti = new TemplateInfo();
                ti->SourceRect = roi;
                try {
                    m_surf(graphData.m_CommonData->m_imCorrectedGpu8UC1, GpuMat(), ti->KeypointsGpu, ti->DescriptorsGpu);
                    ti->TemplateValid = true;
                    m_surf.downloadKeypoints(ti->KeypointsGpu, ti->KeypointsCpu);
                    m_surf.downloadDescriptors(ti->DescriptorsGpu, ti->DescriptorsCpu);
                }
                catch (...) {
                    ti->TemplateValid = false;
                }
                templates.push_back(*ti);
            }
            return templates;
        }

        std::vector<MatcherInfo> Matcher::MatchDescriptors (std::vector<TemplateInfo>&object, std::vector<TemplateInfo>&scene)
        {
            std::vector<MatcherInfo> matcherInfos;
            assert(object.size() == scene.size());

            for (int k = 0; k < object.size(); k++) {
                MatcherInfo* mip = new MatcherInfo();
                MatcherInfo &mi = *mip;
                mi.good_matches = 0;                // if 0, no available matches

                auto &obj = object.at(k);
                auto &scn = scene.at(k);
                MatchDetails md;

                if (obj.TemplateValid && scn.TemplateValid) {
                    auto maxSize = max(obj.DescriptorsCpu.size(), scn.DescriptorsCpu.size());
                    // FSCK!!! BFMatcher will cause later segfaults if matches is not preallocated!!!   FSCK!!!
                    m_DMatches.resize(int(maxSize));
                    m_matcherBF->match(scn.DescriptorsGpu, obj.DescriptorsGpu, m_DMatches);
#if 1
                    //-- Quick calculation of max and min distances between keypoints
                    double max_dist = 0; double min_dist = 100000;

                    for (int i = 0; i < m_DMatches.size(); i++)
                    {
                        double dist = m_DMatches[i].distance;
                        if (dist < min_dist) min_dist = dist;
                        if (dist > max_dist) max_dist = dist;
                    }
                    cout << "-- Max dist : " << max_dist << std::endl;
                    cout << "-- Min dist : " << min_dist << std::endl;

                    //-- Collect only "good" matches (i.e. whose distance is less than 3*min_dist )
                    for (int i = 0; i < m_DMatches.size(); i++)
                    {
                        double dist = m_DMatches[i].distance;
                        if ((dist < 3 * min_dist) || (dist < 0.25))
                        {
                            md.GoodMatches.push_back(cv::DMatch(m_DMatches[i]));
                        }
                    }
                    
                    std::vector<Point2f> objPoints;
                    std::vector<Point2f> scnPoints;

                    //-- Localize the object
                    for (size_t i = 0; i < md.GoodMatches.size(); i++)
                    {
                        //-- Get the keypoints from the good matches
                        auto match = cv::DMatch( md.GoodMatches[i]);
                        objPoints.push_back(Point2f(obj.KeypointsCpu[match.trainIdx].pt));
                        scnPoints.push_back(Point2f(scn.KeypointsCpu[match.queryIdx].pt));
                        scn.GoodKeypointsCpu.push_back(cv::KeyPoint(scn.KeypointsCpu[match.queryIdx]));
                    }

                    Mat H;
                    H = estimateRigidTransform(objPoints, scnPoints, false);

                    if (!H.empty()) {
                        float dx = (float)H.at<double>(0, 2);
                        float dy = (float)H.at<double>(1, 2);
                        mi.dX = obj.SourceRect.tl().x - dx;
                        mi.dY = obj.SourceRect.tl().y - dy;
                        mi.distance = sqrt(mi.dX * mi.dX + mi.dY * mi.dY);
                        mi.rotation = atan2f(mi.dY, mi.dX);
                        mi.good_matches = (int)md.GoodMatches.size();
                    }
#endif
                }
                matcherInfos.push_back(mi);
            }
            return matcherInfos;
        }


        ProcessResult Matcher::process(GraphData& graphData) override
        {
            std::unique_lock<std::mutex> lck(m_mtx);

            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_8UC1, true);

            if (graphData.m_CommonData->m_imCorrectedGpu8UC1.empty()) {
                return ProcessResult::OK;
            }

            SelectAlgorithms();
            bool haveNewTemplate = false;

            if (m_GrabNewTemplate) {
                // always get a new Template in TEMCA mode
                m_GrabNewTemplate = m_bTemcaMode;

                auto objInfos = FindDescriptorsAndKeypoints(graphData);
                m_ObjTemplateInfos.assign(objInfos.begin(), objInfos.end());
                if (!m_bTemcaMode && !m_ObjTemplateInfos[0].TemplateValid) {
                    m_GrabNewTemplate = true; // force reacquistion if not in temca mode and no template found
                }
                else {
                    haveNewTemplate = true;
                }
            }
            if (m_bTemcaMode) {
                // todo, create list of templateInfos from TLBR images
            }
            else if (!haveNewTemplate) {
                auto infos = FindDescriptorsAndKeypoints(graphData);
                m_ScnTemplateInfos.assign(infos.begin(), infos.end());
                m_MatcherInfos = MatchDescriptors(m_ObjTemplateInfos, m_ScnTemplateInfos);
                if (m_MatcherInfos[0].good_matches > 0) {
                    cout << "-- good_matches: " << m_MatcherInfos[0].good_matches  << std::endl;
                }
            }
            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void Matcher::processView(GraphData & graphData) override
        {
            if (m_showView) {

                graphData.EnsureFormatIsAvailable(false, CV_8UC1, true);                // we always need CPU 8 bit
                m_imView = graphData.m_CommonData->m_imCorrected8UC1;

                ClearOverlayText();

                std::ostringstream str;
                int posLeft = 10;
                double scale = 1.0;
                float units_per_pixel = 1;

                switch (m_FieldToView) {
                case 0: // pixel units displayed
                case 1: // real world units displayed
                    for (int i = 0; i < m_MatcherInfos.size(); i++) {
                        if ((m_MatcherInfos[i].good_matches > 0) && (m_ScnTemplateInfos[i].GoodKeypointsCpu.size() > 0)) {
                            drawKeypoints(graphData.m_CommonData->m_imCorrected8UC1, 
                                m_ScnTemplateInfos[i].GoodKeypointsCpu, m_imView, Scalar(255, 0, 0),
                                DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

                            //if (!m_H.empty() && (m_SceneCorners.size() == 4)) {
                            //    //-- Draw lines between the corners (the mapped object in the m_ScenePoints - image_2 )
                            //    cv::line(m_imView, m_SceneCorners[0], m_SceneCorners[1], Scalar(0, 255, 0), 8);
                            //    cv::line(m_imView, m_SceneCorners[1], m_SceneCorners[2], Scalar(0, 255, 0), 8);
                            //    cv::line(m_imView, m_SceneCorners[2], m_SceneCorners[3], Scalar(0, 255, 0), 8);
                            //    cv::line(m_imView, m_SceneCorners[3], m_SceneCorners[0], Scalar(0, 255, 0), 8);

                            //    // draw displacement line
                            //    Point2f centerObj = Point2f(m_SceneCorners[0].x + m_TemplateSizeX / 2, m_SceneCorners[0].y + m_TemplateSizeY / 2);
                            //    Point2f centerScene = graphData.m_CommonData->m_imCapture.size() / 2;
                            //    cv::line(m_imView, centerObj, centerScene, Scalar(0, 255, 0), 8);

                            //}
                        }
                        if (m_FieldToView == 0) {
                            str << "matches, pixels";
                        }
                        else {
                            units_per_pixel = m_UnitsPerPixel;
                            str << "matches, " << m_UnitsPerPixel << " " << m_Units << "/pixel";
                        }
                        break;
                    }
                case 2:
                    //drawMatches(Mat(m_imTemplate), m_ObjectKeypoints,
                    //    Mat(graphData.m_CommonData->m_imCorrected8UC1),
                    //    m_SceneKeypoints, m_good_matches,
                    //    m_imView,
                    //    Scalar::all(-1), Scalar::all(-1),
                    //    std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
                    str << "good matches";
                    break;

                case 3:
//                    drawKeypoints(m_imTemplate, m_ObjectKeypoints, m_imView, Scalar(255, 0, 0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                    str << "template";
                    break;

                }

                if (m_MatcherInfos.size() > 0) {
                    DrawOverlayText(str.str(), Point(posLeft, 30), scale, CV_RGB(128, 128, 128));

                    str.str("");
                    str << "dX: " << fixed << left << setprecision(2) << setw(14) << m_MatcherInfos[0].dX * units_per_pixel;
                    DrawOverlayText(str.str(), Point(posLeft, 300), scale);

                    str.str("");
                    str << "dY: " << fixed << left << setprecision(2) << setw(14) << m_MatcherInfos[0].dY * units_per_pixel;
                    DrawOverlayText(str.str(), Point(posLeft, 340), scale);

                    str.str("");
                    str << "dist: " << fixed << left << setprecision(2) << setw(14) << m_MatcherInfos[0].distance * units_per_pixel;
                    DrawOverlayText(str.str(), Point(posLeft, 400), scale);

                    str.str("");
                    auto rotDegrees = RAD2DEG(m_MatcherInfos[0].rotation);
                    str << "ang: " << fixed << left << setprecision(2) << setw(14) << ((m_FieldToView == 0) ? m_MatcherInfos[0].rotation : rotDegrees);
                    DrawOverlayText(str.str(), Point(posLeft, 440), scale);

                    str.str("");
                    str << "'N' to grab new template";
                    DrawOverlayText(str.str(), Point(posLeft, 500), scale);
                }
                Filter::processView(graphData);
            }
        }



#if false
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
#endif

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

        virtual void grabMatcherTemplate(int x, int y, int width, int height) { 
            std::unique_lock<std::mutex> lck(m_mtx);
            m_bTemcaMode = false;
            m_GrabNewTemplate = true;
            m_ROIs.clear();
            m_ROIs.push_back(Rect2i(x, y, width, height));
        }

        virtual void grabMatcherEdgeTemplates(int borderPix) { 
            std::unique_lock<std::mutex> lck(m_mtx);
            m_bTemcaMode = true;
            m_BorderPix = borderPix;
            m_ROIs.clear();
            //m_ROIs.push_back(Rect2i(0, 0, borderPix, imageHeight));
            //m_ROIs.push_back(Rect2i(0, 0, imageWidth, borderPix));
            //m_ROIs.push_back(Rect2i(imageWidth - borderPix, 0, borderPix, imageHeight));
            //m_ROIs.push_back(Rect2i(0, imageHeight - borderPix, imageWidth, borderPix));
        }

        virtual MatcherInfo getMatcherInfo() {
            std::unique_lock<std::mutex> lck(m_mtx);
            return m_MatcherInfos[0];
        }

        virtual vector<MatcherInfo> getMatcherInfoEdges() {
            std::unique_lock<std::mutex> lck(m_mtx);
            return m_MatcherInfos;
        }
    private:
        //cv::cuda::GpuMat m_imTemplateGpu;                     
        std::mutex m_mtx;
        SURF_CUDA m_surf;
        //GpuMat m_ObjectKeypointsGPU, m_SceneKeypointsGPU;
        //GpuMat m_ObjectDescriptorsGPU, m_SceneDescriptorsGPU;

        bool m_bTemcaMode = false;      // if True, every new image has all 4 edges processed, else, just use ROI
        int m_BorderPix = 400;          // Size of the border to process when MbTemcaMode == True
        std::vector<cv::Rect> m_ROIs;   // Single ROI in non-TEMCA mode, else Left, Top, Right, Bottom
        int m_Rows;                     // Temca mode
        int m_Cols;                     // Temca mode

        std::vector<TemplateInfo> m_ObjTemplateInfos;
        std::vector<TemplateInfo> m_ScnTemplateInfos;
        std::vector<MatcherInfo> m_MatcherInfos;
        std::vector<DMatch> m_DMatches;

        //Mat m_imTemplate;                           // 32F
        //float m_dX = 0;
        //float m_dY = 0;
        //float m_distance = 0;
        //float m_Rotation = 0;

        //vector<KeyPoint> m_ObjectKeypoints, m_SceneKeypoints;
        //vector<float> m_ObjectDescriptors, m_SceneDescriptors;

        //std::vector<Point2f> m_ObjectPoints;
        //std::vector<Point2f> m_ScenePoints;
        //std::vector<Point2f> m_SceneCorners;

        //Point2f m_TemplatePosTopLeft;

        //std::vector< DMatch > m_good_matches;
        //vector<KeyPoint> m_good_scene_keypoints;

        //vector<std::vector<cv::DMatch> > m_matches2D;

        Ptr<cv::cuda::DescriptorMatcher> m_matcherBF;
        Ptr<cv::cuda::DescriptorMatcher> m_matcherFlann;
        Ptr<cv::cuda::DescriptorMatcher> m_matcherKNN;

        //Mat m_H;                            // homography
        int m_DescriptorAlgo = 0;           // Algo to create descriptors
        int m_MatcherAlgo = 0;              // Algo to perform matches

        bool m_GrabNewTemplate = true;
        int m_FieldToView = 0;                      // 0 is match vectors, 1 is template, 2 is current
        int m_TemplateSizeX = 256;
        int m_TemplateSizeY = 256;
        int m_HessianThreshold = 400;
        int m_HessianThresholdLast = -1;
        float m_UnitsPerPixel = 1;
        std::string m_Units = "nM";

    };
}
