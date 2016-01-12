#pragma once

#include "..\stdafx.h"
#include "xiApi.h"

using namespace std;
using namespace cv;
using namespace cuda;

namespace openCVGraph
{

#define MAX_MIMEA_FOCUS_STEPS 4         // Defines size of slider and size of steps, plus or minus
#define PAGE_LOCKED_MEMORY 0            // page lock the main capture buffer
#define XI_TIMEOUT 5000                 // No operation should take longer than 5 seconds
#define LIMIT_BANDWIDTH_TO_FIX_DOTS_UPPER_RIGHT 1

    // Filter which hosts the Ximea 20MPix camera without using OpenCV capture

    class CamXimea : public CamDefault {
    public:
        CamXimea(std::string name, GraphData& graphData,
            int sourceFormat = CV_16UC1,
            int width = 512, int height = 512)
            : CamDefault(name, graphData, sourceFormat, width, height)
        {
            m_Logger = graphData.m_Logger;
            m_image.size = sizeof(XI_IMG);
        }

        void LogErrors(XI_RETURN stat, string msg) {
            if (stat != XI_OK) {
                m_Logger->error("Ximea error: " + msg);
                m_InitOK = false;
            }
        }

        //Allocate resources if needed
        bool init(GraphData& graphData) override
        {
            // call the base to read/write configs
            Filter::init(graphData);

            graphData.m_CommonData->m_NeedCV_16UC1 = true;

            // need 8 bit for our own view
            if (m_showView) {
                graphData.m_CommonData->m_NeedCV_8UC1 = true;
            }

            XI_RETURN stat;
            stat = xiOpenDevice(camera_index, &m_xiH);
            if (stat == XI_OK) {

                // 3840x3840
                stat = xiSetParamInt(m_xiH, XI_PRM_WIDTH, 3840);
                LogErrors(stat, "XI_PRM_WIDTH");
                stat = xiSetParamInt(m_xiH, XI_PRM_OFFSET_X, 640);
                LogErrors(stat, "XI_PRM_OFFSET_X");

                // 16bpp
                stat = xiSetParamInt(m_xiH, XI_PRM_IMAGE_DATA_FORMAT, XI_MONO16);
                LogErrors(stat, "XI_PRM_IMAGE_DATA_FORMAT");

#if LIMIT_BANDWIDTH_TO_FIX_DOTS_UPPER_RIGHT
                stat = xiSetParamInt(m_xiH, XI_PRM_LIMIT_BANDWIDTH, 5000);
                LogErrors(stat, "XI_PRM_LIMIT_BANDWIDTH");
#endif
                
                // AutoGain off
                stat = xiSetParamInt(m_xiH, XI_PRM_AEAG, 0);
                LogErrors(stat, "XI_PRM_AEAG");

                // Enable lens control
                stat = xiSetParamInt(m_xiH, XI_PRM_LENS_MODE, 1);
                LogErrors(stat, "XI_PRM_LENS_MODE");

                // Init exposure and gain
                stat = xiSetParamInt(m_xiH, XI_PRM_EXPOSURE, m_exposure);
                LogErrors(stat, "XI_PRM_EXPOSURE");
                stat = xiSetParamFloat(m_xiH, XI_PRM_GAIN, (float) (m_gain / 1000.0));
                // for some reason, this sometimes errors so ignore it during init.  
                // LogErrors(stat, "XI_PRM_GAIN");

                if (m_minimumBuffers) {
                    // Limit size of buffer pool 
                    // Without this, the driver will allocate an extra unused 1GB of buffer space!
                    stat = xiSetParamInt(m_xiH, XI_PRM_ACQ_BUFFER_SIZE, 5120 * 3840 * 2 * 4); // (x * y * bytes_per_pixel * buffers)
                    LogErrors(stat, "XI_PRM_ACQ_BUFFER_SIZE");

                    // only 2 buffers (ping and pong), although we should only really need 1
                    stat = xiSetParamInt(m_xiH, XI_PRM_BUFFERS_QUEUE_SIZE, 2);
                    LogErrors(stat, "XI_PRM_BUFFERS_QUEUE_SIZE");
                    
                    // always get the most recent frame captured
                    stat = xiSetParamInt(m_xiH, XI_PRM_RECENT_FRAME, 1);
                    LogErrors(stat, "XI_PRM_RECENT_FRAME");
                }

                if (m_SoftwareTrigger) {
                    // software trigger mode
                    stat = xiSetParamInt(m_xiH, XI_PRM_TRG_SOURCE, XI_TRG_SOFTWARE);
                    LogErrors(stat, "XI_PRM_TRG_SOURCE");
                }

                // flash LED during acquisition?
                stat = xiSetParamInt(m_xiH, XI_PRM_LED_MODE, m_LEDMode);
                LogErrors(stat, "XI_PRM_LED_MODE");

                // perform a single frame test capture to verify operation before streaming
                stat = xiStartAcquisition(m_xiH);
                LogErrors(stat, "xiStartAcquisition");

                if (m_SoftwareTrigger) {
                    // trigger
                    stat = xiSetParamInt(m_xiH, XI_PRM_TRG_SOFTWARE, 1);
                    LogErrors(stat, "XI_PRM_TRG_SOFTWARE");
                }

#if PAGE_LOCKED_MEMORY
                // Create page locked memory to speed CUDA Xfers by 2X
                // DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER 

                // If Pinned memory, we provide the buffer to Ximea (called SAFE because they won't overwrite)
                stat = xiSetParamInt(m_xiH, XI_PRM_BUFFER_POLICY, XI_BP_SAFE);
                LogErrors(stat, "XI_PRM_BUFFER_POLICY");

                m_PageLockedHostMem = HostMem(Size(3840, 3840), CV_16UC1);
                graphData.m_CommonData->m_imCapture = m_PageLockedHostMem.createMatHeader();

                prepareXimeaBuffer();

                stat = xiGetImage(m_xiH, XI_TIMEOUT, &m_image);
                LogErrors(stat, "xiGetImage");
#else
                // We're doing software triggering, so buffers won't get overwritten in UNSAFE mode
                stat = xiSetParamInt(m_xiH, XI_PRM_BUFFER_POLICY, XI_BP_UNSAFE);
                LogErrors(stat, "XI_PRM_BUFFER_POLICY");

                stat = xiGetImage(m_xiH, XI_TIMEOUT, &m_image);
                LogErrors(stat, "xiGetImage"); 
                copyCaptureImage(graphData);
#endif

                // CUDA takes a long time initializing during the first data transfer, so do one here
#if !PAGE_LOCKED_MEMORY
                copyCaptureImage(graphData);
#endif
                // always bump up to full 16 bit range
                //graphData.m_CommonData->m_imCapture *= 16;

                graphData.CopyCaptureToRequiredFormats();
            }
            else {
                LogErrors(stat, "xiOpenDevice" + camera_index);
                return false;
            }

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
            return m_InitOK;
        }

        void prepareXimeaBuffer()
        {
            m_image = { 0 };
            m_image.size = sizeof(XI_IMG);

            m_image.bp = m_PageLockedHostMem.data;
            m_image.bp_size = DWORD(m_PageLockedHostMem.step * m_PageLockedHostMem.rows);
            m_image.padding_x = DWORD(m_PageLockedHostMem.step - (m_PageLockedHostMem.cols * m_PageLockedHostMem.elemSize()));
        }

        // bugbug, todo. Capture directly to CUDA contiguous buffer...
        void copyCaptureImage(GraphData& graphData) {
            graphData.m_CommonData->m_imCapture = Mat(m_image.width, m_image.height, CV_16UC1, m_image.bp);
        }

        ProcessResult process(GraphData& graphData) override
        {
            m_firstTime = false;
            bool fOK = true;
            XI_RETURN stat;

            if (m_SoftwareTrigger) {
                stat = xiSetParamInt(m_xiH, XI_PRM_TRG_SOFTWARE, 1);
                LogErrors(stat, "XI_PRM_TRG_SOFTWARE");
            }

#if PAGE_LOCKED_MEMORY
            prepareXimeaBuffer();
#endif
            stat = xiGetImage(m_xiH, XI_TIMEOUT, &m_image);
            LogErrors(stat, "xiGetImage");

#if !PAGE_LOCKED_MEMORY
            copyCaptureImage(graphData);
#endif
            // always bump up to full 16 bit range
            //graphData.m_CommonData->m_imCapture *= 16;

            graphData.CopyCaptureToRequiredFormats();

            if (m_focusMovementStepSize != 0) {
                FocusStep();
            }

            return ProcessResult::OK;
        }

        void processView(GraphData& graphData) override
        {
            if (m_showView) {
                m_imView = graphData.m_CommonData->m_imCapture * 16;
                Filter::processView(graphData);
            }
        }

        // deallocate resources
        bool fini(GraphData& graphData) override
        {
            if (m_xiH != NULL) {
                xiStopAcquisition(m_xiH);
                xiCloseDevice(m_xiH);
            }
            return true;
        }

        void  saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "camera_index" << camera_index;
            fs << "trigger_software" << m_SoftwareTrigger;
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
            fs["trigger_software"] >> m_SoftwareTrigger;
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
            //fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVE, 0);                    // do the move
            XI_RETURN stat = xiSetParamInt(m_xiH, XI_PRM_LENS_FOCUS_MOVE, 0);
            LogErrors(stat, "XI_PRM_LENS_FOCUS_MOVE");
            m_Logger->info("Focus stepped");
            return fOK;
        }

        // v is pos or neg offset from zero
        void FocusViaSlider(int v) {
            bool fOK = true;
            m_focusMovementStepSize = m_minFocusMovementValue * v;
            XI_RETURN stat = xiSetParamInt(m_xiH, XI_PRM_LENS_FOCUS_MOVEMENT_VALUE, m_focusMovementStepSize);
            LogErrors(stat, "XI_PRM_LENS_FOCUS_MOVEMENT_VALUE");
            // fOK = cap.set(CV_CAP_PROP_XI_LENS_FOCUS_MOVEMENT_VALUE, m_focusMovementStepSize);   // may be pos or net
            fOK = FocusStep();
            m_Logger->info("Focus moved: " + std::to_string(m_focusMovementStepSize));
        }

        void Aperature(int v) {
            bool fOK = true;
            m_aperture = v;
            XI_RETURN stat = xiSetParamFloat(m_xiH, XI_PRM_LENS_APERTURE_VALUE, (float)(m_aperture / 1000.0));
            LogErrors(stat, "XI_PRM_LENS_APERTURE_VALUE");
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
                XI_RETURN stat = xiSetParamFloat(m_xiH, XI_PRM_GAIN XI_PRMM_DIRECT_UPDATE, (float) (m_gain / 1000.));
                LogErrors(stat, "XI_PRM_GAIN");
                m_Logger->info("Gain " + std::to_string(m_gain));
            }
        }
        int getExposure() override {
            return m_exposure;
        }
        void setExposure(int value) override {
            m_exposure = value;
            XI_RETURN stat = xiSetParamInt(m_xiH, XI_PRM_EXPOSURE  XI_PRMM_DIRECT_UPDATE, m_exposure);
            LogErrors(stat, "XI_PRM_EXPOSURE");
            m_Logger->info("Exposure " + std::to_string(m_exposure));
        }

    private:
        bool m_InitOK = true;
        XI_IMG m_image = { 0 };
        HANDLE m_xiH = NULL;
        HostMem m_PageLockedHostMem;
        bool m_SoftwareTrigger = true;

        bool m_isAutoGain = false;
        bool m_minimumBuffers = true;
        int m_gain = 1000;                  // * 1000
        int m_gainSliderMax = 7500;         // * 1000
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
        int m_LEDMode = XI_LED_EXPOSURE_ACTIVE;
        std::shared_ptr<logger> m_Logger;

        static void ExposureCallback(int pos, void * userData) {
            CamXimea* camXimea = (CamXimea *)userData;
            camXimea->setExposure(pos);
        }

        static void GainCallback(int pos, void * userData) {
            CamXimea* camXimea = (CamXimea *)userData;
            camXimea->setGain(pos);
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

