#include "..\stdafx.h"
#include "CamXimea.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
    CamXimea::CamXimea(std::string name, GraphData& graphData, int width, int height)
        : CamDefault(name, graphData, width, height)
    {
    }

    // keyWait required to make the UI activate
    bool CamXimea::processKeyboard(GraphData& data, int key)
    {
        bool fOK = true;
        if (m_showView) {

            //switch (key) {
            //    // EXPOSURE
            //case 0x00260000: // up arrow
            //    Exposure(true);
            //    break;
            //case 0x00280000: // down arrow
            //    Exposure(false);
            //    break;

            //    // GAIN
            //case 0x00270000: // RIGHT arrow
            //    Gain(true);
            //    break;
            //case 0x00250000: // LEFT arrow
            //    Gain(false);
            //    break;

            //    // FOCUS
            //case '.':
            //    Focus(true);
            //    break;
            //case ',':
            //    Focus(false);
            //    break;
            //}

        }

        return fOK;
    }

    void CamXimea::ExposureCallback(int pos, void * userData) {
        CamXimea* camXimea = (CamXimea *)userData;
        camXimea->Exposure(pos);
    }

    void CamXimea::GainCallback(int pos, void * userData) {
        CamXimea* camXimea = (CamXimea *)userData;
        camXimea->Gain(pos);
    }

    void CamXimea::FocusCallback(int pos, void * userData) {
        CamXimea* camXimea = (CamXimea *)userData;
        camXimea->Focus(pos);
    }

    void CamXimea::ApertureCallback(int pos, void * userData) {
        CamXimea* camXimea = (CamXimea *)userData;
        camXimea->Aperature(pos);
    }

    //Allocate resources if needed
    bool CamXimea::init(GraphData& graphData)
    {
        // call the base to read/write configs
        Filter::init(graphData);

        // need 8 bit for our own view
        if (m_showView) {
            graphData.m_NeedCV_8UC1 = true;
        }

        bool fOK = false;

        fOK = cap.open(camera_index);
        if (fOK) {
            // set camera specific properties
            if (fOK) {
                fOK = cap.set(CV_CAP_PROP_XI_IMAGE_DATA_FORMAT, m_is16bpp ? XI_MONO16 : XI_MONO8);

                if (m_isSquare) {
                    // only capture 3840x3840@16bpp
                    fOK = cap.set(CV_CAP_PROP_FRAME_WIDTH, 3840);
                    fOK = cap.set(CV_CAP_PROP_XI_OFFSET_X, 640);
                }

                // Limit the number of buffers
                if (m_minimumBuffers) {
                    fOK = cap.set(CV_CAP_PROP_XI_BUFFERS_QUEUE_SIZE, (double)2);
                    fOK = cap.set(CV_CAP_PROP_XI_RECENT_FRAME, 1);
                }
                // Autogain off
                fOK = cap.set(CV_CAP_PROP_XI_AEAG, 0);

                // Enable aperature and focus
                //cap.set(CV_CAP_PROP_XI_LENS_MODE, 1);
                //m_focalDistance = (int) (1000 * cap.get(CV_CAP_PROP_XI_LENS_FOCUS_DISTANCE));
                //m_focalLength = (int)(1000 * cap.get(CV_CAP_PROP_XI_LENS_FOCAL_LENGTH));
                //m_aperatureValue = (int)(1000 * (cap.get(CV_CAP_PROP_XI_LENS_APERTURE_VALUE));

                fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, 10);
                m_focusMovementValue = cap.get(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE);

                fOK = cap.set(CV_CAP_PROP_XI_EXPOSURE, m_exposure);
                fOK = cap.set(CV_CAP_PROP_XI_GAIN, m_gain / 1000);

                if (m_showView) {
                    if (m_showExposureSlider) {
                        createTrackbar("Exposure", m_CombinedName, &m_exposure, 200000, ExposureCallback, this);
                    }
                    if (m_showGainSlider) {
                        createTrackbar("Gain", m_CombinedName, &m_gain, 10000, GainCallback, this);
                    }
                    if (m_showFocusSlider) {
                        createTrackbar("Focus", m_CombinedName, &m_focalDistance, 1000000, FocusCallback, this);
                    }
                    if (m_showApertureSlider) {
                        createTrackbar("Aperture", m_CombinedName, &m_aperture, 22000, ApertureCallback, this);
                    }
                }
            }
            fOK = cap.read(graphData.m_imCapture);

            if (!graphData.m_imCapture.data)   // Check for invalid input
            {
                fOK = false;
                std::cout << "Could not open capture device #" << camera_index << std::endl;
            }
            else {
                source = Camera;
                fOK = true;
            }
        }


        return fOK;
    }


    ProcessResult CamXimea::process(GraphData& graphData)
    {
        m_firstTime = false;
        bool fOK = true;

        fOK = cap.read(graphData.m_imCapture);

        if (graphData.m_imCapture.depth() == CV_16U) {
            // make 16bpp full range
            graphData.m_imCapture *= 16;
            graphData.m_imCapture16U = graphData.m_imCapture;
            if (graphData.m_NeedCV_8UC1) {
                graphData.m_imCapture16U.convertTo(graphData.m_imCapture8U, CV_8U, 1.0 / 256);
            }
        }
        else {
            graphData.m_imCapture8U = graphData.m_imCapture;
            if (graphData.m_NeedCV_16UC1) {
                graphData.m_imCapture8U.convertTo(graphData.m_imCapture16U, CV_16U, 256);
            }
        }

        graphData.m_imResult8U = graphData.m_imCapture8U;
        graphData.m_imResult16U = graphData.m_imCapture16U;


        // Get the capture image onto the GPU
        graphData.m_imCaptureGpu16U.upload(graphData.m_imCapture16U);
        graphData.m_imCaptureGpu16U.convertTo(graphData.m_imCaptureGpu32F, CV_32F);
        graphData.m_imCaptureGpu16U.convertTo(graphData.m_imCaptureGpu8U, CV_8U, 0.00390625);  // 1/256 scale factor

        // And update the Result images
        graphData.m_imCaptureGpu16U.copyTo(graphData.m_imResultGpu16U);
        if (graphData.m_NeedCV_32FC1) {
            graphData.m_imCaptureGpu32F.copyTo(graphData.m_imResultGpu32F);
        }
        graphData.m_imCaptureGpu8U.copyTo(graphData.m_imResultGpu8U);

        return ProcessResult::OK;
    }
    
    void CamXimea::processView(GraphData& graphData)
    {
        if (m_showView) {
            m_imView = graphData.m_imCapture8U;
            Filter::processView(graphData);
        }
    }

    // deallocate resources
    bool CamXimea::fini(GraphData& graphData)
    {
        if (cap.isOpened()) {
            cap.release();
        }
        return true;
    }



    void  CamXimea::saveConfig(FileStorage& fs, GraphData& data) 
    {
        Filter::saveConfig(fs, data);
        fs << "camera_index" << camera_index;
        fs << "minimum_buffers" << m_minimumBuffers;
        fs << "focus" << m_focalDistance;
        fs << "focalLength" << m_focalLength;
        fs << "aperture" << m_aperture;
        fs << "focusMovementValue" << m_focusMovementValue;
        fs << "exposure" << m_exposure;
        fs << "gain" << m_gain;
        fs << "show_gain_slider" << m_showGainSlider;
        fs << "show_exposure_slider" << m_showExposureSlider;
        fs << "show_focus_slider" << m_showFocusSlider;
        fs << "show_aperature_slider" << m_showApertureSlider;
    }

    void  CamXimea::loadConfig(FileNode& fs, GraphData& data) 
    {
        Filter::loadConfig(fs, data);
        fs["camera_index"] >> camera_index;
        fs["minimum_buffers"] >> m_minimumBuffers;
        fs["focus"] >> m_focalDistance;
        fs["focalLength"] >> m_focalLength;
        fs["aperture"] >> m_aperture;
        fs["focusMovementValue"] >> m_focusMovementValue;
        fs["exposure"] >> m_exposure;
        fs["gain"] >> m_gain;
        fs["show_gain_slider"] >> m_showGainSlider;
        fs["show_exposure_slider"] >> m_showExposureSlider;
        fs["show_focus_slider"] >> m_showFocusSlider;
        fs["show_aperature_slider"] >> m_showApertureSlider;
    }
}

