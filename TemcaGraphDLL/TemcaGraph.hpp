#pragma once

// OpenCVGraphDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Exported.hpp"

using namespace std;
using namespace openCVGraph;

// -----------------------------------------------------------------------------------
// Called at the completion of each loop through a graph to check for user input.
// -----------------------------------------------------------------------------------

bool graphCallback(GraphManager* graphManager)
{
    // waitKey is required in OpenCV to make graphs display, 
    // so this funtion call is required only when "ShowView" is true for at least one graph.

    int key = cv::waitKey(1);
    if (graphManager->AbortOnEscape())
    {
        if (key == 27) // ESCAPE
        {
            graphManager->getGraphData()->m_Logger->error("graph aborted by user.");
            graphManager->Abort();
            return false;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------------
// The graphs.  Graphs are run in parallel, each with own thread.
// -----------------------------------------------------------------------------------






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
        GraphManager *graph = new GraphManager("GraphCamXimea", true, graphCallback, m_graphCommonData);

        CvFilter camera(new CamXimea("CamXimea", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(camera);

#ifdef WITH_CUDA
        CvFilter fbrightDark(new CapturePostProcessing("CapturePostProcessing", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(fbrightDark);
#endif
        return graph;
    }

    GraphManager* CreateGraphCamXimeaDummy()
    {
        // Create a graph
        GraphManager *graph = new GraphManager("GraphCamXimeaDummy", true, graphCallback, m_graphCommonData);

        CvFilter camera(new CamDefault("CamXimeaDummy", *graph->getGraphData(), StreamIn::CaptureRaw, 512, 512));
        graph->AddFilter(camera);

#ifdef WITH_CUDA
        CvFilter fbrightDark(new CapturePostProcessing("CapturePostProcessing", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(fbrightDark);
#endif
        return graph;
    }

    // -----------------------------------------------------------------------------------
    // Post capture graphs
    // -----------------------------------------------------------------------------------


    GraphManager* CreateGraphCapturePostprocessing()
    {
        // Create a graph
        GraphManager *graph = new GraphManager("GraphCapturePostProcessing", true, graphCallback, m_graphCommonData);

#ifdef WITH_CUDA
        CvFilter fCapPost(new CapturePostProcessing("CapturePostProcessing", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(fCapPost);
#endif
        return graph;
    }

    GraphManager* CreateGraphFileWriter()
    {
        // Create a graph
        GraphManager *graph = new GraphManager("GraphFileWriter", true, graphCallback, m_graphCommonData, false);

        CvFilter fileWriter(new FileWriter("FileWriter", *graph->getGraphData(), StreamIn::Corrected));
        graph->AddFilter(fileWriter);

        return graph;
    }

    GraphManager* CreateGraphQC()
    {
        // Create a graph
        GraphManager *graph = new GraphManager("GraphQC", true, graphCallback, m_graphCommonData, true);

#ifdef WITH_CUDA
        CvFilter filter(new openCVGraph::ImageQC("ImageQC", *graph->getGraphData(), StreamIn::CaptureRaw));
        graph->AddFilter(filter);

        CvFilter fFocusFFT(new FocusFFT("FocusFFT", *graph->getGraphData(), StreamIn::Corrected, 512, 512));
        graph->AddFilter(fFocusFFT);

#endif
        return graph;
    }


    GraphManager* CreateGraphStitchingCheck()
    {
        // Create a graph
        GraphManager *graph = new GraphManager("GraphStitchingCheck", true, graphCallback, m_graphCommonData, true);

        // todo, bugbug fix
        CvFilter filter(new Delay("Delay", *graph->getGraphData()));
        graph->AddFilter(filter);

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

            m_Logger->info("Temca starting up -----------------------------------");
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Log creation failed: " << ex.what() << std::endl;
        }
    }

    bool init(bool fDummyCamera, StatusCallbackType callback)
    {
        m_fDummyCamera = fDummyCamera;

        bool fOK = true;
        m_PythonCallback = callback;

        m_gmCapture = m_fDummyCamera ? CreateGraphCamXimeaDummy() : CreateGraphCamXimea();
        m_gmFileWriter = CreateGraphFileWriter();
        m_gmQC = CreateGraphQC();
        m_gmStitchingCheck = CreateGraphStitchingCheck();

        m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> { m_gmCapture });
        m_StepPostCapture = new GraphParallelStep("StepPostCapture", list<GraphManager*> { m_gmFileWriter, m_gmQC, m_gmStitchingCheck });

        // Create a list of all steps
        m_Steps.push_back(m_StepCapture);
        m_Steps.push_back(m_StepPostCapture);

        // and just those steps following capture
        m_StepsPostCapture.push_back(m_StepPostCapture);

        // init each step
        for (auto step : m_Steps) {
            fOK = step->init();
            if (!fOK) {
                m_Logger->error("init of " + step->GetName() + " failed!");
                return fOK;
            }
        }

        // ITemcaCamera, ITemcaFocus, etc..
        FindTemcaInterfaces();

        return fOK;
    }

    // Sets the overall mode of operation for the Temca graph.
    //    Each mode reconfigures or deactivates parts of the overall temca graph.

    //    temca :         ximea, lshift4, postCap, QC
    //                                             Focus
    //                                             FW
    //                                             Stitch
    //    
    //    capture_raw :   ximea, lshift4,          FW
    //    preview :       ximea, lshift4,
    //    auto_exposure : ximea, lshift4,          QC

    bool setMode(const char * graphType)
    {
        bool fOK = true;
        string sGraphType(graphType);

        // Create the graphs
        if (sGraphType == "temca") {

            // Each step runs to completion.  
            // Each graph in a step runs in parallel with other graphs in the step.
            m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> { m_gmCapture });
            m_StepPostCapture = new GraphParallelStep("StepPostCapture", list<GraphManager*> { m_gmFileWriter, m_gmQC, m_gmStitchingCheck });
        }
        else if (sGraphType == "dummy") {

            // Each step runs to completion.  
            // Each graph in a step runs in parallel with other graphs in the step.
            m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> { m_gmCapture });
            m_StepPostCapture = new GraphParallelStep("StepPostCapture", list<GraphManager*> { m_gmFileWriter, m_gmQC, m_gmStitchingCheck });
            // ... could have more steps here
        }
        else if (sGraphType == "camera_only") {
            //m_gmCapture = GraphCamXimea(m_graphCommonData, false);  // no CapturePostProcessing

            // Each step runs to completion.  
            // Each graph in a step runs in parallel with other graphs in the step.
            m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> { m_gmCapture });
            m_StepPostCapture = new GraphParallelStep("StepPostCapture", list<GraphManager*> {});
        }
        else if (sGraphType == "delay") {
            // Experiment with adding delays at various points in the graph
            auto g1 = CreateGraphDelay("DelayCap", 2000);
            // fake out QC and Stitching
            auto g2 = CreateGraphDelay("Delay1", 2000);
            auto g3 = CreateGraphDelay("Delay2", 2000);

            // Each step runs to completion.  
            // Each graph in a step runs in parallel with other graphs in the step.
            m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> { g1 });
            m_StepPostCapture = new GraphParallelStep("StepPostCapture", list<GraphManager*> { g2, g3 });
        }

        // Create a list of all steps
        m_Steps.push_back(m_StepCapture);
        m_Steps.push_back(m_StepPostCapture);

        // and just those steps following capture
        m_StepsPostCapture.push_back(m_StepPostCapture);

        return fOK;
    }

    void fini() {
        bool fOK = true;
        for (auto step : m_Steps) {
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
        m_Logger->info("Temca fini ------------------------------------------------");
    }

    // -----------------------------------------------------------------
    // Python control interfaces
    // -----------------------------------------------------------------

    void GrabFrame(const char * filename, int roiX, int roiY)
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_CompletedStep = false;
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

        memcpy(image, gd->m_CommonData->m_imCapture.data, size_t(gd->m_CommonData->m_imCapture.size().area() * sizeof(UINT16)));
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
        FocusInfo info;
        if (m_ITemcaFocus) {
            info = m_ITemcaFocus->getFocusInfo();
        }
        return info;
    }

    // ------------------------------------------------
    // ITemcaQC
    // ------------------------------------------------
    QCInfo getQCInfo() {
        QCInfo info;
        if (m_ITemcaQC) {
            info = m_ITemcaQC->getQCInfo();
        }
        return info;
    }

private:
    bool m_fDummyCamera;

    // The graphs which  can run simultaneous on separate threads, 
    // and either on GPU or CPU
    GraphManager* m_gmCapture = NULL;
    GraphManager* m_gmFileWriter = NULL;
    GraphManager* m_gmQC = NULL;
    GraphManager* m_gmStitchingCheck = NULL;

    GraphCommonData *m_graphCommonData = new GraphCommonData();

    // Bundled graphs which step together
    GraphParallelStep* m_StepCapture = NULL;
    GraphParallelStep* m_StepPostCapture = NULL;

    std::list<GraphParallelStep*> m_Steps;
    std::list<GraphParallelStep*> m_StepsPostCapture;

    bool m_Enabled = true;
    std::atomic_bool m_Aborting = false;

    std::thread m_thread;
    std::atomic_bool m_Stepping = false;

    std::mutex m_mtx;                                   
    std::condition_variable m_cv;                       // 
    std::atomic_bool m_CompletedStep = false;           // Has the step finished?
    //std::atomic_bool m_CompletedRun = false;            // Has the run finished?

    int m_LogLevel = spd::level::info;
    std::shared_ptr<spdlog::logger> m_Logger;

    string m_CaptureFileName;

    // control interfaces
    ITemcaCamera* m_ITemcaCamera = NULL;
    ITemcaFocus* m_ITemcaFocus = NULL;
    ITemcaQC* m_ITemcaQC = NULL;

    // callback to python
    StatusCallbackInfo m_PythonInfo = { 0 };
    StatusCallbackType m_PythonCallback = NULL;

    void FindTemcaInterfaces()
    {
        for (auto step : m_Steps) {
            for (auto graph : step->m_Graphs) {
                for (auto processor : graph->GetFilters()) {
                    // the next line took me 2 hours to figure out!!!
                    if (dynamic_cast<ITemcaCamera *> (processor.get()) != nullptr)
                    {
                        m_ITemcaCamera = dynamic_cast<ITemcaCamera *> (processor.get());
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
                }
            }
        }
    }

    bool PythonCallback(int status, int error, const char * errorString) {
        bool fOK = true;
        if (m_PythonCallback) {
            m_PythonInfo.status = status;
            m_PythonInfo.info_code = error;
            strcpy_s(m_PythonInfo.error_string, errorString);
            // Keep going if the python callback returns True
            fOK = ((m_PythonCallback)(&m_PythonInfo) != 0);
        }
        return fOK;
    }

    enum StatusCodes {
        FatalError = -1,            // Aborting processing loop.
        InitFinished = 0,           // Ready to party, dude!
        StartNewFrame = 1,          // Ready for client to issue get_image()
        CaptureFinished = 2,        // Capture completed, ready to step the stage
        SyncStepCompleted = 3,      // A synchronous processing step post capture has completed
        AsyncStepCompleted = 4,     // An asynchronous processing step post capture has completed
        ProcessingFinished = 5,     // At the end of the loop.  All steps except ASYNC steps have completed
        ShutdownFinished = 6        // Police just showed up.
    };

    // The main capture loop
    // After the capture step, there are two kinds of additional steps:
    //   Async - which can run until the END of the next capture.
    //   Sync - which are awaited in the loop.

    bool ProcessLoop()
    {
        bool fOK = true;
        string s;

        fOK = PythonCallback(InitFinished, 0, "");

        try {
            while (fOK && !m_Aborting) {
                m_CompletedStep = false;
                fOK &= PythonCallback(StartNewFrame, 0, "");

                // Wait for the client to issue a grab, which sets m_Stepping

                std::unique_lock<std::mutex> lk(m_mtx);
                m_cv.wait(lk, [=]() {return (m_Stepping == true) || m_Aborting; });  // return false to continue waiting!
                m_Stepping = false;

                if (!m_Aborting) {

                    // Step CAPTURE
                    if (!(fOK = m_StepCapture->Step())) {
                        s = m_StepCapture->GetName() + " failed Capture Step.";
                        m_Logger->error(s);
                        PythonCallback(FatalError, 0, s.c_str());
                        m_Aborting = true;
                    }
                    else {
                        // Wait for CAPTURE to complete
                        if (!(fOK &= m_StepCapture->WaitStepCompletion())) {
                            s = m_StepCapture->GetName() + " Capture WaitStepCompletion.";
                            m_Logger->error(s);
                            PythonCallback(FatalError, 0, s.c_str());
                            m_Aborting = true;
                        }
                        else {
                            // fire finished capture event
                            fOK &= PythonCallback(CaptureFinished, 0, "");

                            // Verify all downstream ASYNC steps have finished from the LAST capture (overlapped case)
                            for (auto step : m_StepsPostCapture) {
                                if (fOK) {
                                    if (step->RunningAsync()) {
                                        if (!(fOK &= step->WaitStepCompletion())) {
                                            s = m_StepCapture->GetName() + " failed WaitStepCompletion.";
                                            m_Logger->error(s);
                                            PythonCallback(FatalError, 0, s.c_str());
                                            m_Aborting = true;
                                            break;
                                        }
                                        else {
                                        }
                                    }
                                }
                            }
                            //PythonCallback(AsyncStepCompleted, 0, s.c_str());

                            // step all of the post capture steps
                            for (auto step : m_StepsPostCapture) {
                                if (!(fOK &= step->Step())) {
                                    s = m_StepCapture->GetName() + " failed to Step.";
                                    m_Logger->error(s);
                                    PythonCallback(FatalError, 0, s.c_str());
                                    m_Aborting = true;
                                    break;
                                }
                            }
                            // and now wait for all the steps to complete
                            for (auto step : m_StepsPostCapture) {
                                if (fOK) {
                                    if (!(fOK &= step->WaitStepCompletion())) {
                                        s = m_StepCapture->GetName() + " failed WaitStepCompletion.";
                                        m_Logger->error(s);
                                        PythonCallback(FatalError, 0, s.c_str());
                                        m_Aborting = true;
                                        break;
                                    }
                                    else {
                                        // todo PythonCallback(SyncStepCompleted, 0, s.c_str());

                                    }
                                }
                            }
                        }
                    }
                }
                fOK &= PythonCallback(ProcessingFinished, 0, "");
                m_CompletedStep = true;
            }
            fini(); // cleanup
        }
        catch (exception& ex)
        {
            std::cout << "main processing loop: " << ex.what() << std::endl;
        }
        fOK &= PythonCallback(ShutdownFinished, 0, "");
        return fOK;
    }


};

// ---------------------------------------------------------------------------------
// ctypes doesn't interface to C++, so duplicate the above in straight C.  DOH!
// ---------------------------------------------------------------------------------

Temca * pTemca = NULL;

bool open(bool fDummyCamera, StatusCallbackType callback)
{
    pTemca = new Temca();

    bool fOK = pTemca->init(fDummyCamera, callback);
    if (fOK) {
        pTemca->StartThread();
    }
    return fOK;
}

bool close()
{
    if (pTemca) {
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

// ------------------------------------------------------
// ITemcaCamera
// ------------------------------------------------------



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