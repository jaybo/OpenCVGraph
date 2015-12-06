#include "..\stdafx.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{
#define MAX_MIMEA_FOCUS_STEPS 4  // Defines size of slider and size of steps, plus or minus

    class CamXimea : public CamDefault {
    public:
        CamXimea(std::string name, GraphData& graphData,
            int sourceFormat,
            int width, int height)
            : CamDefault(name, graphData, sourceFormat, width, height)
        {
            m_Logger = graphData.m_Logger;
        }

        // keyWait required to make the UI activate
        bool processKeyboard(GraphData& data, int key) override
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



        //Allocate resources if needed
        bool init(GraphData& graphData) override
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


        ProcessResult process(GraphData& graphData) override
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

        void processView(GraphData& graphData) override
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
        bool fini(GraphData& graphData) override
        {
            if (cap.isOpened()) {
                cap.release();
            }
            return true;
        }



        void  saveConfig(FileStorage& fs, GraphData& data) override
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

        void  loadConfig(FileNode& fs, GraphData& data) override
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

        //void Trigger(TRIGGER trigger) {
        //    bool fOK = true;
        //    triggerType = trigger;
        //    switch (triggerType)
        //    {
        //    case FreeRun:
        //        // no trigger, use internal camera clock to run as fast as you can Forrest
        //        isGrabOnly = false;
        //        fOK = cap.set(XI_TRG_OFF, 0);
        //        Log(fOK, "FreeRun");
        //        break;
        //    case ManualGrab:
        //        // Wait for key hit
        //        isGrabOnly = true;
        //        fOK = cap.set(CV_CAP_PROP_XI_TRG_SOFTWARE, XI_TRG_SEL_FRAME_START);
        //        Log(fOK, "ManualGrab");
        //        break;
        //    case SoftwareTrigger:
        //        // We're telling you when to capture buddy
        //        isGrabOnly = false;
        //        fOK = cap.set(CV_CAP_PROP_XI_TRG_SOFTWARE, XI_TRG_SEL_FRAME_START);
        //        Log(fOK, "SoftwareTrigger");
        //        break;
        //    }
        //}

        //// Initiate acqusition via software
        //void TriggerSoftwareCapture() {
        //    bool fOK = true;
        //    fOK = cap.set(CV_CAP_PROP_XI_TRG_SOFTWARE, XI_TRG_SEL_FRAME_START);
        //}

        //// get a new frame from camera
        //void Grab() {
        //    cap >> frame;
        //}

        void Exposure(bool up) {
            bool fOK = true;
            if (!m_isAutoGain) {
                int inc = 1000;
                m_exposure += up ? inc : -inc;
                m_exposure = max(m_exposure, 1000);    // arbitrary: 1mS is the lowest we go
                cap.set(CV_CAP_PROP_XI_EXPOSURE, m_exposure);
                m_Logger->info("Exposure " + std::to_string(m_exposure));
            }
        }

        void Exposure(int v) {
            bool fOK = true;
            if (!m_isAutoGain) {
                m_exposure = v;
                cap.set(CV_CAP_PROP_XI_EXPOSURE, m_exposure);
                m_Logger->info("Exposure " + std::to_string(m_exposure));
            }
        }

        void Gain(bool up) {
            bool fOK = true;
            if (!m_isAutoGain) {
                int inc = 250;                // == 0.25
                m_gain += up ? inc : -inc;
                m_gain = max(m_gain, 1000);
                m_gain = min(m_gain, 100000);
                cap.set(CV_CAP_PROP_XI_GAIN, m_gain / 1000);
                m_Logger->info("Gain " + std::to_string(m_gain));
            }
        }

        void Gain(int v) {
            bool fOK = true;
            if (!m_isAutoGain) {
                m_gain = v;
                fOK = cap.set(CV_CAP_PROP_XI_GAIN, m_gain / 1000);
                m_Logger->info("Gain " + std::to_string(m_gain));
            }
        }

        bool FocusStep() {
            bool fOK = true;
            fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVE, 0);                    // do the move
            m_Logger->info("Focus stepped");
            return fOK;
        }

        // v is pos or neg offset from zero
        void FocusViaSlider(int v) {
            bool fOK = true;
            m_focusMovementStepSize = m_minFocusMovementValue * v;
            fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, m_focusMovementStepSize);   // may be pos or net
            fOK = FocusStep();
            m_Logger->info("Focus moved: " + std::to_string(m_focusMovementStepSize));
        }

        void Aperature(int v) {
            bool fOK = true;
            m_aperture = v;
            fOK = cap.set(CV_CAP_PROP_XI_LENS_APERTURE_VALUE, m_aperture / 1000);
            m_Logger->info("Aperature " + std::to_string(m_aperture));
        }

    private:

        bool m_isAutoGain = false;
        bool m_is16bpp = true;
        bool m_isSquare = true;
        bool m_minimumBuffers = true;
        int m_gain = 1000;                  // * 1000
        int m_gainSliderMax = 10000;        // * 1000
        int m_aperture = 1200;              // * 1000
        int m_focusMovementSliderPos = MAX_MIMEA_FOCUS_STEPS;
        int m_focusMovementStepSize = 0;
        int m_minFocusMovementValue = 2;
        int m_exposure = 10000;             // in microseconds
        int m_exposureSliderMax = 260000;         // for setting the slider max
        bool m_showGainSlider = false;
        bool m_showExposureSlider = false;
        bool m_showFocusSlider = false;
        bool m_showApertureSlider = false;
        std::shared_ptr<logger> m_Logger;

        static void ExposureCallback(int pos, void * userData) {
            CamXimea* camXimea = (CamXimea *)userData;
            camXimea->Exposure(pos);
        }

        static void GainCallback(int pos, void * userData) {
            CamXimea* camXimea = (CamXimea *)userData;
            camXimea->Gain(pos);
        }

        static void FocusCallback(int pos, void * userData) {
            CamXimea* camXimea = (CamXimea *)userData;
            camXimea->m_focusMovementSliderPos = pos;
            camXimea->FocusViaSlider(pos - MAX_MIMEA_FOCUS_STEPS);
        }

        static void ApertureCallback(int pos, void * userData) {
            CamXimea* camXimea = (CamXimea *)userData;
            camXimea->Aperature(pos);
        }
    };
}
