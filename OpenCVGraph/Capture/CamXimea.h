
#pragma once
#pragma warning(disable : 4482)

#include "..\stdafx.h"
#include "CamXimea.h"


namespace openCVGraph
{
#define MAX_MIMEA_FOCUS_STEPS 4  // Defines size of slider and size of steps, plus or minus

    class CamXimea : public CamDefault
    {
    public:
        CamXimea(std::string name, GraphData& data, int width = 512, int height = 512);

        virtual bool init(GraphData& graphData) override;
        virtual bool fini(GraphData& graphData) override;

        virtual ProcessResult process(GraphData& graphData) override;
        virtual bool processKeyboard(GraphData& data, int key) override;
        virtual void processView(GraphData& graphData) override;

        virtual void saveConfig(FileStorage& fs, GraphData& data) override;
        virtual void loadConfig(FileNode& fs, GraphData& data) override;

        void Log(bool fOK, string s)
        {
            string t = s + (fOK ? string("") : string(" ERROR --------------------------")) + "\n";
            printf(t.c_str());

        }

        void Focus(bool up) {
            bool fOK = true;
            if (true) {
                cap.set(CV_CAP_PROP_XI_LENS_MODE, XI_ON);
                int inc = 2;
                fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, up ? inc : -inc);
                fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVE, 0);// up ? inc : -inc);
                Log(fOK, "Focus " + std::to_string(up));
                // cap.set(CV_CAP_PROP_XI_LENS_MODE, XI_OFF);
            }
        }

        void AutoGain(bool useAutoGain) {
            bool fOK = true;
            m_isAutoGain = useAutoGain;
            if (m_isAutoGain) {
                fOK = cap.set(CV_CAP_PROP_XI_AEAG, 1);
                Log(fOK, "AutoGain");
            }
            else {
                fOK = cap.set(CV_CAP_PROP_XI_AEAG, 0);
                Log(fOK, "ManualGain");
            }
        }

        void SquareImage(bool squareImage) {
            bool fOK = true;
            m_isSquare = squareImage;
            if (m_isSquare) {
                // only capture 3840x3840
                fOK = cap.set(CV_CAP_PROP_FRAME_WIDTH, 3840);
                fOK = cap.set(CV_CAP_PROP_XI_OFFSET_X, 640);
                Log(fOK, "Square Image");
            }
            else {
                // capture 5120x3840
                fOK = cap.set(CV_CAP_PROP_XI_OFFSET_X, 0);
                fOK = cap.set(CV_CAP_PROP_FRAME_WIDTH, 5120);
                Log(fOK, "Full Image");

            }
        }

        void Bpp16(bool bpp16) {
            bool fOK = true;
            m_is16bpp = bpp16;
            if (m_is16bpp) {
                fOK = cap.set(CV_CAP_PROP_XI_IMAGE_DATA_FORMAT, XI_MONO16);
                Log(fOK, "16 bpp");
            }
            else {
                fOK = cap.set(CV_CAP_PROP_XI_IMAGE_DATA_FORMAT, XI_MONO8);
                Log(fOK, "8 bpp");
            }
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
                Log(fOK, "Exposure " + std::to_string(m_exposure));
            }
        }

        void Exposure(int v) {
            bool fOK = true;
            if (!m_isAutoGain) {
                m_exposure = v;
                cap.set(CV_CAP_PROP_XI_EXPOSURE, m_exposure);
                Log(fOK, "Exposure " + std::to_string(m_exposure));
            }
        }

        void Gain(bool up) {
            bool fOK = true;
            if (!m_isAutoGain) {
                int inc = 250;                // == 0.25
                m_gain += up ? inc : -inc;
                m_gain = max(m_gain, 1000);
                m_gain = min(m_gain, 100000);
                cap.set(CV_CAP_PROP_XI_GAIN, m_gain/1000);
                Log(fOK, "Gain " + std::to_string(m_gain));
            }
        }

        void Gain(int v) {
            bool fOK = true;
            if (!m_isAutoGain) {
                m_gain = v;
                fOK = cap.set(CV_CAP_PROP_XI_GAIN, m_gain / 1000);
                Log(fOK, "Gain " + std::to_string(m_gain));
            }
        }

        bool FocusStep() {
            bool fOK = true;
            fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVE, 0);                    // do the move
            Log(fOK, "Focus stepped");
            return fOK;
        }

        // v is pos or neg offset from zero
        void FocusViaSlider(int v) {
            bool fOK = true;
            m_focusMovementStepSize = m_minFocusMovementValue * v;
            fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, m_focusMovementStepSize);   // may be pos or net
            fOK = FocusStep();
            Log(fOK, "Focus moved: " + std::to_string(m_focusMovementStepSize));
        }

        void Aperature(int v) {
            bool fOK = true;
            m_aperture = v;
            fOK = cap.set(CV_CAP_PROP_XI_LENS_APERTURE_VALUE, m_aperture / 1000);
            Log(fOK, "Aperature " + std::to_string(m_aperture));
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

        static void ExposureCallback(int pos, void * userData);
        static void GainCallback(int pos, void * userData);
        static void FocusCallback(int pos, void * userData);
        static void ApertureCallback(int pos, void * userData);
    };
}

