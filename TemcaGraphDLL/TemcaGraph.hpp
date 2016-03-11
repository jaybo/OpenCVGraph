#pragma once

// OpenCVGraphDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Exported.hpp"
#ifdef _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif

using namespace std;
using namespace openCVGraph;
using namespace std::placeholders;

#define ADD_DELAYS 0                    // Add delay filters to sync and async steps to test logic
#define FOCUS_AND_QC_IN_ONE_GRAPH 1     // OpenCV Bug forces this?

// -----------------------------------------------------------------------------------
// Optionally called at the completion of each loop through a graph to check for user input.
// -----------------------------------------------------------------------------------

bool graphCallback(GraphManager* graphManager)
{
    // waitKey is required in OpenCV to make graphs display, 
    // so this funtion call is required.
    GraphCommonData * gcd = graphManager->getGraphData()->m_CommonData;

    if (graphManager->AbortOnEscape())
    {
        if (gcd->m_LastKey == 27) // ESCAPE
        {
            graphManager->getGraphData()->m_Logger->error("graph aborted by user.");
            graphManager->Abort();
            return false;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------------
// The Temca Class used by Python
// -----------------------------------------------------------------------------------

class Temca
{
private:

    // -----------------------------------------------------------------------------------
    // Capture graphs
    // -----------------------------------------------------------------------------------

    GraphManager* CreateGraphCamXimea()
    {
        // Create a graph
        GraphManager *graph = new GraphManager("Temca-CamXimea", true, graphCallback, m_graphCommonData);

        CvFilter camera(new CamXimea("CamXimea", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(camera);

        return graph;
    }

    GraphManager* CreateGraphCamXimeaDummy(const char * dummyPath)
    {
        // Create a graph
        GraphManager *graph = new GraphManager("Temca-CamDummy", true, graphCallback, m_graphCommonData);
        bool useDirectory = false;
        bool useFile = false;
        bool useMovie = false;
        bool useCam = false;
        int camIndex = -2;

        vector<string> movieExts = { "mov", "avi", "mpg", "mp4", "mpeg" };

        if ((dummyPath != NULL) && strlen(dummyPath) > 2) {
            std::string p(dummyPath);
            auto fExt = GetFileExtension(p);
            for (auto ext : movieExts) {
                if (ext == fExt) {
                    if (fileExists(dummyPath)) {
                        useMovie = true;
                    }
                }
            }
            if (useMovie)
                ;
            else if (useDirectory = dirExists(dummyPath))
                ;
            else if (useFile = fileExists(dummyPath))
                ;
            else if (isdigit(dummyPath[0])) {
                camIndex = atoi(dummyPath);
                useCam = true;
            }
        }
        CvFilter camera(new CamDefault("CamXimeaDummy", *graph->getGraphData(), StreamIn::CaptureRaw, 512, 512,
            CV_16UC1, 
            useCam ? camIndex : -2, 
            useFile ? dummyPath : "", 
            useMovie ? dummyPath : "",
            useDirectory ? dummyPath : "", 
            true /*dummy*/));
        graph->AddFilter(camera);

        return graph;
    }

    // -----------------------------------------------------------------------------------
    // CapturePostProcessing graph
    // -----------------------------------------------------------------------------------

    GraphManager* CreateGraphCapturePostprocessing(string graphName, 
        int enableBrightDarkCorrection = FROM_YAML, int enableUpShift = FROM_YAML)
    {
        // Create a graph
        GraphManager *graph = new GraphManager(graphName, true, graphCallback, m_graphCommonData);

        CvFilter fCapPost(new CapturePostProcessing("CapturePostProcessing", *graph->getGraphData(), 
            StreamIn::CaptureRaw, 
            512, 512, 
            enableBrightDarkCorrection,
            enableUpShift));
        graph->AddFilter(fCapPost);

        return graph;
    }

    // -----------------------------------------------------------------------------------
    // Post capture graphs
    // -----------------------------------------------------------------------------------

    GraphManager* CreateGraphFileWriter(string graphName)
    {
        // Create a graph
        GraphManager *graph = new GraphManager(graphName, true, graphCallback, m_graphCommonData, false);

        CvFilter fileWriter(new FileWriter("FileWriter", *graph->getGraphData(), StreamIn::Corrected));
        graph->AddFilter(fileWriter);

        return graph;
    }

    GraphManager* CreateGraphQC(string graphName)
    {
        // Create a graph
        GraphManager *graph = new GraphManager(graphName, true, graphCallback, m_graphCommonData, true);

        CvFilter filter(new openCVGraph::ImageQC("ImageQC", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(filter);
        
#if FOCUS_AND_QC_IN_ONE_GRAPH
        CvFilter fFocusFFT(new FocusFFT("FocusFFT", *graph->getGraphData(), StreamIn::Corrected, 512, 512));
        graph->AddFilter(fFocusFFT);
#endif

        return graph;
    }

    GraphManager* CreateGraphFocus(string graphName)
    {
        // Create a graph
        GraphManager *graph = new GraphManager(graphName, true, graphCallback, m_graphCommonData, true);

#if !FOCUS_AND_QC_IN_ONE_GRAPH
        CvFilter fFocusFFT(new FocusFFT("FocusFFT", *graph->getGraphData(), StreamIn::Corrected, 512, 512));
        graph->AddFilter(fFocusFFT);
#endif
        return graph;
    }

    GraphManager* CreateGraphStitchingCheck(string graphName)
    {
        // Create a graph
        GraphManager *graph = new GraphManager(graphName, true, graphCallback, m_graphCommonData, true);
#ifdef WITH_CUDA
        CvFilter fmatcher(new Matcher("Matcher", *graph->getGraphData(), openCVGraph::Corrected, 768, 768));
        graph->AddFilter(fmatcher);
#endif
        return graph;
    }

    GraphManager* CreateGraphDelay (string graphName, int delay=10)
    {
        GraphManager* graph = new GraphManager(graphName, true, graphCallback, m_graphCommonData);

        CvFilter filter(new Delay("Delay", *graph->getGraphData(), StreamIn::CaptureRaw, 0, 0, delay));
        graph->AddFilter(filter);

        return graph;
    }

public:
    Temca() {
        try
        {
            // Set up logging
            const char * loggerName = "TemcaLog";
            // Use existing logger if already created
            if (auto logger = spd::get(loggerName)) {
                m_Logger = logger;
            }
            else {
                string logDir = "logs";
                createDir(logDir);
                std::vector<spdlog::sink_ptr> sinks;
                sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
                sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(logDir + "/" + loggerName, "txt", 23, 59));
                m_Logger = std::make_shared<spdlog::logger>(loggerName, begin(sinks), end(sinks));
                spdlog::register_logger(m_Logger);
            }

            m_Logger->info("Temca starting up ----------------------------------------------------------------------");
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Log creation failed: " << ex.what() << std::endl;
        }
    }

    // Startup the Temca graph, with either a real or dummy camera
    bool init (bool fDummyCamera, const char * dummyPath, StatusCallbackType callback)
    {
        m_fDummyCamera = fDummyCamera;

        bool fOK = true;
        m_PythonCallback = callback;

        // Create the single capture graph containing the camera
        m_gmCapture = m_fDummyCamera ? CreateGraphCamXimeaDummy(dummyPath) : CreateGraphCamXimea();

        // Create the post processing graph
        // Assume dummy camera images are already upshifted and corrected.
        m_gmCapturePostProcessing = CreateGraphCapturePostprocessing("Temca-CapturePostProcessing",
            (int)!m_fDummyCamera /*enable BrightDark correction*/,
            (int)!m_fDummyCamera /*enableUpShift*/);

        m_gmFileWriter = CreateGraphFileWriter("Temca-FileWriter");
        m_gmFocus = CreateGraphFocus("Temca-Focus");
        m_gmQC = CreateGraphQC("Temca-QC");
        m_gmStitching = CreateGraphStitchingCheck("Temca-Stitching");
        m_gmDelaySync = CreateGraphDelay("Temca-DelaySync", 2000);
        m_gmDelayAsync = CreateGraphDelay("Temca-DelayAsync", 3000);

        // ------------------------------------------------------------------------------
        // The singular capture step, which is always in use
        // ------------------------------------------------------------------------------
        
        m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> () =
        { 
            m_gmCapture 
        }, StatusCaptureFinished, false /* runAsync */, (tPythonCallbackFunction) std::bind<bool>(&Temca::PythonCallback, this, _1, _2, _3));
        m_AllSteps.push_back(m_StepCapture); // always only one capture step

        // ------------------------------------------------------------------------------
        // The singular CapturePostProcessing step, which is always in use
        // ------------------------------------------------------------------------------

        m_StepCapturePostProcessing = new GraphParallelStep("StepCapturePostProcessing", list<GraphManager*>() =
        {
            m_gmCapturePostProcessing
        }, StatusCapturePostProcessingFinished, false /* runAsync */, (tPythonCallbackFunction)std::bind<bool>(&Temca::PythonCallback, this, _1, _2, _3));
        m_AllSteps.push_back(m_StepCapturePostProcessing); // always only one capture post processing step

        // ------------------------------------------------------------------------------
        // Main TEMCA CAPTURE configuration
        // ------------------------------------------------------------------------------

        m_StepSync = new GraphParallelStep("StepsTemcaSync", list<GraphManager*>() =
        {
            m_gmQC,
#if !FOCUS_AND_QC_IN_ONE_GRAPH
            m_gmFocus,
#endif
            m_gmFileWriter,
#if ADD_DELAYS
            m_gmDelaySync,
#endif
        }, StatusSyncStepFinished, false /* runAsync */, (tPythonCallbackFunction)std::bind<bool>(&Temca::PythonCallback, this, _1, _2, _3));
        m_StepsPostCapture.push_back(m_StepSync);

        m_StepAsync = new GraphParallelStep("StepsTemcaAsync", list<GraphManager*> () =
        {
            m_gmStitching,
#if ADD_DELAYS
            m_gmDelayAsync,
#endif
        }, StatusAsyncStepFinished, true /*runAsync*/, (tPythonCallbackFunction)std::bind<bool>(&Temca::PythonCallback, this, _1, _2, _3));
        m_StepsPostCapture.push_back(m_StepAsync);

        m_AllSteps.insert(m_AllSteps.end(), m_StepsPostCapture.begin(), m_StepsPostCapture.end());

        // init all of the steps
        for (auto step : m_AllSteps) {
            fOK = step->init();
            if (!fOK) {
                m_Logger->error("init of " + step->GetName() + " failed!");
                return fOK;
            }
        }

        // ITemcaCamera, ITemcaFocus, etc..
        FindTemcaInterfaces();

        setMode("temca");

        return fOK;
    }

    // ------------------------------------------------------------------------------
    // Sets the overall mode of operation for the Temca graph.
    // Each mode reconfigures or deactivates parts of the overall temca graph.
    //
    //    mode                            SYNC          ASYNC
    //    --------------------------------------------------------------
    //    temca :         ximea, postCap, QC            Stitch
    //                                    Focus
    //                                    FW
    //    
    //    capture_raw :   ximea, postCap, FW
    //    preview :       ximea, postCap, QC
    //                                    Focus
    // ------------------------------------------------------------------------------

    bool setMode(const char * graphType)
    {
        if (!m_CanChangeMode) {
            m_Logger->error("Can't change mode when a step is in progress");
            return false;  
        }
        string sGraphType(graphType);
        m_graphCommonData->m_FrameNumber = 0;

        // Select a sequence of steps to run
        if (sGraphType == "temca") {
            m_gmCapture->Enable(true);
            m_gmCapturePostProcessing->Enable(true);
            m_gmQC->Enable(true);
            m_gmFocus->Enable(true);
            m_gmFileWriter->Enable(true);
            m_gmStitching->Enable(true);
        }
        else if (sGraphType == "raw") {
            m_gmFileWriter->Enable(true);
            m_gmStitching->Enable(false);
        }
        else if (sGraphType == "preview") {
            m_gmFileWriter->Enable(false);
            m_gmStitching->Enable(true);
        }
        else if (sGraphType == "delay") {
        }

        return true;
    }

    void fini() {
        bool fOK = true;
        for (auto step : m_AllSteps) {
            fOK = step->fini();
            if (!fOK) {
                m_Logger->error("fini of " + step->GetName() + " failed!");
            }
        }
    }

    void StartThread()
    {
        m_thread = std::thread::thread(&Temca::ProcessLoop, this);
    }

    void JoinThread()
    {
        m_Aborting = true;
        m_cv.notify_all();
        m_thread.join();
        m_Logger->info("Temca fini ----------------------------------------------------------------------");
    }

    std::mutex s_LockMutex;  // protects Python callback reentrancy

    bool PythonCallback(int status, int error, const char * errorString) {
        std::lock_guard<std::mutex> lock(s_LockMutex);

        bool fOK = true;
        if (m_PythonCallback) {
            
            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure();

            m_PythonInfo.status = status;
            m_PythonInfo.info_code = error;
            strcpy_s(m_PythonInfo.error_string, errorString);
            // Keep going if the python callback returns True
            fOK = ((m_PythonCallback)(&m_PythonInfo) != 0);
            // m_Logger->info("PythonCallback " + to_string(status));

            PyGILState_Release(gstate);
        }
        return fOK;
    }

    // -----------------------------------------------------------------
    // Python control interfaces
    // -----------------------------------------------------------------

    void GrabFrame(const char * filename, int roiX, int roiY)
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_CanChangeMode = false;
        m_Stepping = true;
        m_graphCommonData->m_DestinationFileName = filename;
        m_graphCommonData->m_roiX = roiX;
        m_graphCommonData->m_roiY = roiY;
        m_cv.notify_all();
    }

    void setROIInfo(const ROIInfo * roiInfo) {
        m_graphCommonData->m_ROISizeX = roiInfo->gridX;
        m_graphCommonData->m_ROISizeY = roiInfo->gridY;
    }

    void getLastFrame(UINT16 * image) {
        GraphData * gd = m_gmCapture->getGraphData();
        assert(gd->m_CommonData->m_imCapture.isContinuous());
        int count = gd->m_CommonData->m_imCapture.size().area();
        memcpy(image, gd->m_CommonData->m_imCapture.data, size_t(count * sizeof(UINT16)));
    }

    void getPreviewFrame(UINT8 * image) {
        GraphData * gd = m_gmCapture->getGraphData();
        assert(gd->m_CommonData->m_imPreview.isContinuous());
        int count = gd->m_CommonData->m_imPreview.size().area();
        memcpy(image, gd->m_CommonData->m_imPreview.data, size_t(count * sizeof(UINT8)));
    }

    // Generic function to set parameters on the graph
    void setParameter(const char * parameter, INT32 value)
    {
        string p(parameter);

        if (p == "exposure" && m_ITemcaCamera != NULL) {
            m_ITemcaCamera->setExposure(value);
        }
        else if (p == "gain" && m_ITemcaCamera != NULL) {
            m_ITemcaCamera->setGain(value);
        }
        else if (p == "preview_decimation_factor" && m_ITemcaCapturePostProcessing != NULL) {
            m_ITemcaCapturePostProcessing->setPreviewDecimationFactor(value);
        }

    }

    // Generic function to get parameters on the graph
    INT32 getParameter(const char * parameter)
    {
        INT32 value = 0;
        string p(parameter);

        if (p == "exposure" && m_ITemcaCamera != NULL) {
            value = m_ITemcaCamera->getExposure();
        }
        else if (p == "gain" && m_ITemcaCamera != NULL) {
            value = m_ITemcaCamera->getGain();
        }
        else if (p == "preview_decimation_factor" && m_ITemcaCapturePostProcessing != NULL) {
            value = m_ITemcaCapturePostProcessing->getPreviewDecimationFactor();
        }
        return value;
    }


    // ------------------------------------------------
    // ITemcaCamera 
    // ------------------------------------------------
    CameraInfo getCameraInfo() {
        CameraInfo info = { 0 };
        if (m_ITemcaCamera) {
            info = m_ITemcaCamera->getCameraInfo();
        }
        return info;
    }

    // milliseond units
    int getExposure() {
        int value = -1;
        if (m_ITemcaCamera) {
            value = m_ITemcaCamera->getExposure();
        }
        return value;
    }

    void setExposure(int value) {
        if (m_ITemcaCamera) {
            m_ITemcaCamera->setExposure(value);
        }
    }

    // milliseond units
    int getGain() {
        int value = -1;
        if (m_ITemcaCamera) {
            value = m_ITemcaCamera->getGain();
        }
        return value;
    }

    void setGain(int value) {
        if (m_ITemcaCamera) {
            m_ITemcaCamera->setGain(value);
        }
    }

    // ------------------------------------------------
    // ITemcaFocus 
    // ------------------------------------------------
    FocusInfo getFocusInfo() {
        FocusInfo info = {0};
        if (m_ITemcaFocus) {
            info = m_ITemcaFocus->getFocusInfo();
        }
        return info;
    }

    void setFFTSize(UINT32 dimension, UINT32 startFreq, UINT32 endFreq) {
        if (m_ITemcaFocus) {
            m_ITemcaFocus->setFFTSize(dimension, startFreq, endFreq);
        }
    }

    // ------------------------------------------------
    // ITemcaQC
    // ------------------------------------------------
    QCInfo getQCInfo() {
        QCInfo info = {0};
        if (m_ITemcaQC) {
            info = m_ITemcaQC->getQCInfo();
        }
        return info;
    }

    // ------------------------------------------------
    // ITemcaMatcher
    // ------------------------------------------------
    void grabMatcherTemplate(int x, int y, int width, int height) {
        if (m_ITemcaMatcher) {
            m_ITemcaMatcher->grabMatcherTemplate(x, y, width, height);
        }
    }

    MatcherInfo getMatcherInfo() {
        MatcherInfo info = { 0 };
        if (m_ITemcaMatcher) {
            info = m_ITemcaMatcher->getMatcherInfo();
        }
        return info;
    }

private:
    bool m_fDummyCamera;

    // The graphs which  can run simultaneous on separate threads, 
    // and either on GPU or CPU
    GraphManager* m_gmCapture = NULL;
    GraphManager* m_gmCapturePostProcessing = NULL;
    GraphManager* m_gmFileWriter = NULL;
    GraphManager* m_gmFocus = NULL;
    GraphManager* m_gmQC = NULL;
    GraphManager* m_gmStitching = NULL;
    GraphManager* m_gmDelaySync = NULL;
    GraphManager* m_gmDelayAsync = NULL;

    GraphCommonData *m_graphCommonData = new GraphCommonData();

    // Bundled graphs which step together.  The loop doesn't continue until all graphs in a step individually complete (except the Async graph)
    GraphParallelStep* m_StepCapture = NULL;
    GraphParallelStep* m_StepCapturePostProcessing = NULL;
    GraphParallelStep* m_StepSync = NULL;
    GraphParallelStep* m_StepAsync = NULL;

    std::list<GraphParallelStep*> m_AllSteps;                   // all steps, including camera
    std::list<GraphParallelStep*> m_StepsPostCapture;           // post capture steps 

    bool m_Enabled = true;
    bool m_Aborting = false;

    std::thread m_thread;
    bool m_Stepping = false;

    std::mutex m_mtx;                                   
    std::condition_variable m_cv;                       
    bool m_CanChangeMode = true;           // Has the step finished?

    int m_LogLevel = spd::level::info;
    std::shared_ptr<spdlog::logger> m_Logger;

    string m_CaptureFileName;

    // control interfaces
    ITemcaCamera* m_ITemcaCamera = NULL;
    ITemcaCapturePostProcessing* m_ITemcaCapturePostProcessing = NULL;
    ITemcaFocus* m_ITemcaFocus = NULL;
    ITemcaQC* m_ITemcaQC = NULL;
    ITemcaMatcher* m_ITemcaMatcher = NULL;

    // callback to python
    StatusCallbackInfo m_PythonInfo;
    StatusCallbackType m_PythonCallback = NULL;

    // Find the control interfaces which may shift between filters and graphs 
    // as the configurations change
    void FindTemcaInterfaces()
    {
        m_ITemcaQC = NULL;
        m_ITemcaFocus = NULL;

        for (auto step : m_AllSteps) {
            for (auto graph : step->m_Graphs) {
                for (auto processor : graph->GetFilters()) {
                    // the next line took me 2 hours to figure out!!!
                    if (dynamic_cast<ITemcaCamera *> (processor.get()) != nullptr)
                    {
                        m_ITemcaCamera = dynamic_cast<ITemcaCamera *> (processor.get());
                        continue;
                    }
                    if (dynamic_cast<ITemcaCapturePostProcessing *> (processor.get()) != nullptr)
                    {
                        m_ITemcaCapturePostProcessing = dynamic_cast<ITemcaCapturePostProcessing *> (processor.get());
                        continue;
                    }                    
                    if (dynamic_cast<ITemcaFocus *> (processor.get()) != nullptr)
                    {
                        m_ITemcaFocus = dynamic_cast<ITemcaFocus *> (processor.get());
                        continue;
                    }
                    if (dynamic_cast<ITemcaQC *> (processor.get()) != nullptr)
                    {
                        m_ITemcaQC = dynamic_cast<ITemcaQC *> (processor.get());
                        continue;
                    }
                    if (dynamic_cast<ITemcaMatcher *> (processor.get()) != nullptr)
                    {
                        m_ITemcaMatcher = dynamic_cast<ITemcaMatcher *> (processor.get());
                        continue;
                    }
                }
            }
        }
    }

    // Status events MUST match the numbers of the corresponding Python events
    enum StatusCodes {
        StatusFatalError = -1,                      // Aborting processing loop.  We are dead.
        StatusInitFinished = 0,                     // Ready to party, dude!
        StatusStartNewFrame = 1,                    // Ready for client to issue get_image(). 
        StatusCaptureFinished = 2,                  // Capture completed, ready to step the stage
        StatusCapturePostProcessingFinished = 3,    // *4, BrighDark, Spatial correction finished, preview ready
        StatusSyncStepFinished = 4,                 // A synchronous processing step post capture has completed
        StatusAsyncStepFinished = 5,                // An asynchronous processing step post capture has completed
        StatusShutdownFinished = 6                  // Police just showed up.
    };

    // The main capture loop
    // After the capture step, there are two kinds of additional steps:
    //   Async - which can run until the END of the next capture.
    //   Sync - which are awaited in the loop.

    bool ProcessLoop()
    {
        bool fOK = true;
        string s;

        fOK = PythonCallback(StatusInitFinished, 0, "");

        try {
            while (fOK && !m_Aborting) {
#if 0
                // Verify no heap corruption
                bool heapOK = HeapValidate(GetProcessHeap(), 0, NULL);
                if (!heapOK) {
                    m_Logger->emerg("Heap is corrupted");
                }
#endif
                // see if a keyboard key has been pressed
                m_graphCommonData->PerformWaitKey();

                m_CanChangeMode = true;
                fOK &= PythonCallback(StatusStartNewFrame, 0, "");

                // Wait for the client to issue a grab, which sets m_Stepping

                std::unique_lock<std::mutex> lk(m_mtx);
                m_cv.wait(lk, [=]() {return (m_Stepping == true) || m_Aborting; });  // return false to continue waiting!
                m_Stepping = false;

                if (!m_Aborting) {

                    // Step CAPTURE 
                    if (!(fOK = m_StepCapture->Step())) {
                        s = m_StepCapture->GetName() + " failed Capture Step.";
                        m_Logger->error(s);
                        PythonCallback(StatusFatalError, 0, s.c_str());
                        m_Aborting = true;
                    }
                    else {
                        // Wait for CAPTURE to complete
                        if (!(fOK &= m_StepCapture->WaitStepCompletion())) {
                            s = m_StepCapture->GetName() + " Capture WaitStepCompletion.";
                            m_Logger->error(s);
                            PythonCallback(StatusFatalError, 0, s.c_str());
                            m_Aborting = true;
                        }
                        else {
                            // Verify all downstream ASYNC steps have finished from the LAST capture.
                            // This is the overlapped case, where an exposure is happening while the 
                            // previous frame ASYNC step is still completing
                            for (auto step : m_StepsPostCapture) {
                                if (fOK) {
                                    if (step->RunningAsync()) {
                                        if (!(fOK &= step->WaitStepCompletion(true /*async*/))) {
                                            s = m_StepCapture->GetName() + " failed WaitStepCompletion.";
                                            m_Logger->error(s);
                                            PythonCallback(StatusFatalError, 0, s.c_str());
                                            m_Aborting = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            // Step the singular CapturePostProcessing step

                            if (!(fOK = m_StepCapturePostProcessing->Step())) {
                                s = m_StepCapturePostProcessing->GetName() + " failed CapturePostProcessing Step.";
                                m_Logger->error(s);
                                PythonCallback(StatusFatalError, 0, s.c_str());
                                m_Aborting = true;
                            }
                            else {
                                // Wait for CapturePostProcessing to complete
                                if (!(fOK &= m_StepCapturePostProcessing->WaitStepCompletion())) {
                                    s = m_StepCapture->GetName() + " CapturePostProcessing WaitStepCompletion.";
                                    m_Logger->error(s);
                                    PythonCallback(StatusFatalError, 0, s.c_str());
                                    m_Aborting = true;
                                }
                            }

                            // step all of the post capture steps
                            for (auto step : m_StepsPostCapture) {
                                if (!(fOK &= step->Step())) {
                                    s = m_StepCapture->GetName() + " failed to Step.";
                                    m_Logger->error(s);
                                    PythonCallback(StatusFatalError, 0, s.c_str());
                                    m_Aborting = true;
                                    break;
                                }
                                else {
                                    // But only wait here for the synchronous steps
                                    // (The ASYNC steps are awaited above)
                                    if (!step->RunningAsync()) {
                                        if (!(fOK &= step->WaitStepCompletion())) {
                                            s = m_StepCapture->GetName() + " failed WaitStepCompletion.";
                                            m_Logger->error(s);
                                            PythonCallback(StatusFatalError, 0, s.c_str());
                                            m_Aborting = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            fini(); // cleanup
        }
        catch (exception& ex)
        {
            std::cout << "main processing loop: " << ex.what() << std::endl;
        }
        fOK &= PythonCallback(StatusShutdownFinished, 0, "");
        return fOK;
    }
};

// ---------------------------------------------------------------------------------
// ctypes doesn't interface to C++, so duplicate the above in straight C.  DOH!
//
// Since these are just global entry points to the functions above, refer to the
// above documentation.
// ---------------------------------------------------------------------------------

// The singular global TEMCA object
Temca * pTemca = NULL;

bool temca_open(bool fDummyCamera, const char * dummyPath, StatusCallbackType callback)
{
    pTemca = new Temca();

    bool fOK = pTemca->init(fDummyCamera, dummyPath, callback);
    if (fOK) {
        pTemca->StartThread();
    }
    return fOK;
}

bool temca_close()
{
    if (pTemca) {
        //pTemca->fini();
        pTemca->JoinThread();
    }
    delete pTemca;
    pTemca = NULL;
    return true;
}

bool setMode(const char* graphType)
{
    if (pTemca) {
        return pTemca->setMode(graphType);
    }
    return false;
}

void setParameter(const char * parameter, INT32 value)
{
    if (pTemca) {
        pTemca->setParameter(parameter, value);
    }
}

INT32 getParameter(const char * parameter)
{
    INT32 v = 0;
    if (pTemca) {
        v = pTemca->getParameter(parameter);
    }
    return v;
}

void grabFrame(const char * filename, UINT32 roiX, UINT32 roiY)
{
    if (pTemca) {
        pTemca->GrabFrame(filename, roiX, roiY);
    }
}

CameraInfo getCameraInfo() {
    if (pTemca) {
        return pTemca->getCameraInfo();
    }
    return CameraInfo();
}

FocusInfo getFocusInfo() {
    if (pTemca) {
        return pTemca->getFocusInfo();
    }
    FocusInfo info = { 0 };
    return info;
}

void setFFTSize(UINT32 dimension, UINT32 startFreq, UINT32 endFreq) {
    if (pTemca) {
        return pTemca->setFFTSize(dimension, startFreq, endFreq);
    }
}

QCInfo getQCInfo() {
    if (pTemca) {
        return pTemca->getQCInfo();
    }
    QCInfo info = { 0 };
    return info;
}

StatusCallbackInfo getStatus() {
    StatusCallbackInfo ci;
    ci.status = 42;
    strncpy_s(ci.error_string, "this is not an error", sizeof(ci.error_string) - 1);
    return ci;
}

void setROI(const ROIInfo *  roiInfo) {
    if (pTemca) {
        pTemca->setROIInfo(roiInfo);
    }
}

void getLastFrame(UINT16 * image) {
    if (pTemca) {
        pTemca->getLastFrame(image);
    }
}

void getPreviewFrame(UINT8 * image) {
    if (pTemca) {
        pTemca->getPreviewFrame(image);
    }
}

// ---------------------------------------------------------
// ITemcaMatcher
// ---------------------------------------------------------
void grabMatcherTemplate(int x, int y, int width, int height)
{
    if (pTemca) {
        pTemca->grabMatcherTemplate(x, y, width, height);
    }
}

MatcherInfo getMatcherInfo()
{
    MatcherInfo mi = { 0 };
    if (pTemca) {
        mi = pTemca->getMatcherInfo();
    }
    return mi;
}

