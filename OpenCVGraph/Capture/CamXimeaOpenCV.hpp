#pragma once

#include "..\stdafx.h"

using namespace std;
using namespace cv;

namespace openCVGraph
{

#define MAX_MIMEA_FOCUS_STEPS_OCV 4     // Defines size of slider and size of steps, plus or minus
#define SOFTWARE_TRIGGER_OCV 1          // else free run
#define PINNED_MEMORY_OCV 0             // page lock the main capture buffer

    // Filter which hosts the Ximea 20MPix camera accessed via OpenCV-->DirectShow-->XimeaDriver
    // which is presumably slower than the native version

    class CamXimeaOpenCV : public CamDefault {
    public:
        CamXimeaOpenCV(std::string name, GraphData& graphData,
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : CamDefault(name, graphData, streamIn, width, height)
        {
            m_Logger = graphData.m_Logger;
        }

        //Allocate resources if needed
        bool init(GraphData& graphData) override
        {
            //// call the base to read/write configs
            //Filter::init(graphData);

            //graphData.m_CommonData->m_NeedCV_16UC1 = true;

            //// need 8 bit for our own view
            //if (m_showView) {
            //    graphData.m_CommonData->m_NeedCV_8UC1 = true;
            //}

            bool fOK = false;

            fOK = cap.open(camera_index);
            if (fOK) {
                // set camera specific properties
                if (fOK) {
                    // Limit the number of buffers
                    if (m_minimumBuffers) {
                        fOK = cap.set(CV_CAP_PROP_XI_BUFFERS_QUEUE_SIZE, (double)2);
                        fOK = cap.set(CV_CAP_PROP_XI_RECENT_FRAME, 1);
                    }

                    if (m_isSquare) {
                        // only capture 3840x3840@16bpp
                        fOK = cap.set(CV_CAP_PROP_FRAME_WIDTH, 3840);
                        fOK = cap.set(CV_CAP_PROP_XI_OFFSET_X, 640);
                    }
                    fOK = cap.set(CV_CAP_PROP_XI_IMAGE_DATA_FORMAT, m_is16bpp ? XI_MONO16 : XI_MONO8);

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
#if PINNED_MEMORY_OCV
                // This doesn't work
                //cuda::HostMem page_locked(Size(3840, 3840), CV_16UC1);
                //graphData.m_CommonData->m_imCapture = page_locked.createMatHeader();
#endif

#if SOFTWARE_TRIGGER_OCV
                cap.set(CV_CAP_PROP_XI_TRG_SOURCE, XI_TRG_SOFTWARE);
                cap.set(CV_CAP_PROP_XI_TRG_SOFTWARE, 1);
#endif
                fOK = cap.read(graphData.m_CommonData->m_imCapture);

                if (!graphData.m_CommonData->m_imCapture.data)   // Check for invalid input
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

#if SOFTWARE_TRIGGER_OCV
            cap.set(CV_CAP_PROP_XI_TRG_SOFTWARE, 1);
#endif
            fOK = cap.read(graphData.m_CommonData->m_imCapture);

            if (graphData.m_CommonData->m_imCapture.depth() == CV_16U) {
                // make 16bpp full range
                graphData.m_CommonData->m_imCapture *= 16;
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
                if (graphData.m_CommonData->m_imCapture.depth() == CV_16U) {
                    graphData.m_CommonData->m_imCapture.convertTo(m_imView, CV_8UC1, 1.0 / 256);
                }
                else {
                    m_imView = graphData.m_CommonData->m_imCapture;
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
                auto t = getExposure();
                t += up ? inc : -inc;
                t = max(t, 1000);    // arbitrary: 1mS is the lowest we go
                setExposure(t);
            }
        }

        void Gain(bool up) {
            if (!m_isAutoGain) {
                int inc = 250;                // == 0.25
                auto t = getGain();
                t += up ? inc : -inc;
                t = max(t, 1000);
                t = min(t, 100000);
                setGain(t);
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

        //
        // ITemcaCamera, external I/F accessed by clients
        //
 
        int getGain() override {
            return m_gain;
        }
        void setGain(int value) override {
            if (!m_isAutoGain) {
                m_gain = value;
                cap.set(CV_CAP_PROP_XI_GAIN, m_gain / 1000);
                m_Logger->info("Gain " + std::to_string(m_gain));
            }
        }
        int getExposure() override {
            return m_exposure;
        }
        void setExposure(int value) override {
            m_exposure = value;
            cap.set(CV_CAP_PROP_XI_EXPOSURE, m_exposure);
            m_Logger->info("Exposure " + std::to_string(m_exposure));
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
            CamXimeaOpenCV* camXimea = (CamXimeaOpenCV *)userData;
            camXimea->setExposure(pos);
        }

        static void GainCallback(int pos, void * userData) {
            CamXimeaOpenCV* camXimea = (CamXimeaOpenCV *)userData;
            camXimea->setGain(pos);
        }

        static void FocusCallback(int pos, void * userData) {
            CamXimeaOpenCV* camXimea = (CamXimeaOpenCV *)userData;
            camXimea->m_focusMovementSliderPos = pos;
            camXimea->FocusViaSlider(pos - MAX_MIMEA_FOCUS_STEPS);
        }

        static void ApertureCallback(int pos, void * userData) {
            CamXimeaOpenCV* camXimea = (CamXimeaOpenCV *)userData;
            camXimea->Aperature(pos);
        }
    };
}

