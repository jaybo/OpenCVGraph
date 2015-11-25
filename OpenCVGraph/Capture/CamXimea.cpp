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
        camXimea->m_focusMovementSliderPos = pos;
        camXimea->FocusViaSlider(pos - MAX_MIMEA_FOCUS_STEPS);
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

        graphData.m_NeedCV_16UC1 = true;

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
                cap.set(CV_CAP_PROP_XI_LENS_MODE, 1);
                //m_focalDistance = (int) (1000 * cap.get(CV_CAP_PROP_XI_LENS_FOCUS_DISTANCE));
                //m_focalLength = (int)(1000 * cap.get(CV_CAP_PROP_XI_LENS_FOCAL_LENGTH));
                //m_aperatureValue = (int)(1000 * (cap.get(CV_CAP_PROP_XI_LENS_APERTURE_VALUE));

                
               
                fOK = cap.set(CV_CAP_PROP_XI_EXPOSURE, m_exposure);
                fOK = cap.set(CV_CAP_PROP_XI_GAIN, m_gain / 1000);

                if (m_showView) {
                    if (m_showExposureSlider) {
                        createTrackbar("Exposure", m_CombinedName, &m_exposure, m_exposureSliderMax, ExposureCallback, this);
                    }
                    if (m_showGainSlider) {
                        createTrackbar("Gain", m_CombinedName, &m_gain, m_gainSliderMax, GainCallback, this);
                    }
                    if (m_showFocusSlider) {
                        createTrackbar("Focus", m_CombinedName, &m_focusMovementSliderPos, MAX_MIMEA_FOCUS_STEPS * 2, FocusCallback, this);
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
                graphData.m_Logger->error() << "Could not read from capture device #" << camera_index;
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
        }

        graphData.CopyCaptureToRequiredFormats();
        
        if (m_focusMovementStepSize != 0) {
            FocusStep();
        }

        return ProcessResult::OK;
    }
    
    void CamXimea::processView(GraphData& graphData)
    {
        if (m_showView) {
            // Convert back to 8 bits for the view
            if (graphData.m_imCapture.depth() == CV_16U) {
                graphData.m_imCapture.convertTo(m_imView, CV_8UC1, 1.0 / 256);
            }
            else {
                m_imView = graphData.m_imCapture;
            }
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
        fs << "aperture" << m_aperture;
        fs << "minFocusMovementValue" << m_minFocusMovementValue;
        fs << "exposure" << m_exposure;
        fs << "exposureSliderMax" << m_exposureSliderMax;
        fs << "gain" << m_gain;
        fs << "gainSliderMax" << m_gainSliderMax;
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
        fs["aperture"] >> m_aperture;
        fs["minFocusMovementValue"] >> m_minFocusMovementValue;
        fs["exposure"] >> m_exposure;
        fs["exposureSliderMax"] >> m_exposureSliderMax;
        fs["gain"] >> m_gain;
        fs["gainSliderMax"] >> m_gainSliderMax;
        fs["show_gain_slider"] >> m_showGainSlider;
        fs["show_exposure_slider"] >> m_showExposureSlider;
        fs["show_focus_slider"] >> m_showFocusSlider;
        fs["show_aperature_slider"] >> m_showApertureSlider;
    }
}

