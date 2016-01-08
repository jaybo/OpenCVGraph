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
    // so this funtion call is required.

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

GraphManager* GraphWebCam(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphWebCam", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter camera(new CamDefault("WebCam", *gd, CV_16UC1));
    graph->AddFilter(camera);

    return graph;
}


GraphManager* GraphCamXimea(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphCamXimea", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter camera(new CamXimea("CamXimea", *gd, CV_16UC1));
    graph->AddFilter(camera);

#ifdef WITH_CUDA
    CvFilter fbrightDark(new BrightDarkFieldCorrection("BrightDark", *gd, CV_16UC1));
    graph->AddFilter(fbrightDark);
#endif
    return graph;
}

GraphManager* GraphCamXimeaDummy(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphCamXimeaDummy", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter camera(new CamDefault("CamXimeaDummy", *gd, CV_16UC1, 3840, 3840));
    graph->AddFilter(camera);

#ifdef WITH_CUDA
    CvFilter fbrightDark(new BrightDarkFieldCorrection("BrightDark", *gd, CV_16UC1));
    graph->AddFilter(fbrightDark);
#endif
    return graph;
}

GraphManager* GraphFileWriter(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphFileWriter", true, graphCallback, commonData, false);
    GraphData* gd = graph->getGraphData();

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, CV_16UC1));
    graph->AddFilter(fileWriter);

    return graph;
}

GraphManager* GraphQC(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphQC", true, graphCallback, commonData, true);
    GraphData* gd = graph->getGraphData();

#ifdef WITH_CUDA
    CvFilter fFocusSobel(new FocusSobel("FocusSobel", *gd, CV_16UC1, 512, 150));
    graph->AddFilter(fFocusSobel);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", *gd, CV_16UC1, 512, 512));
    graph->AddFilter(fFocusFFT);

    CvFilter filter(new openCVGraph::ImageStatistics("ImageStatistics", *gd, CV_16UC1));
    graph->AddFilter(filter);
#endif

    return graph;
}

GraphManager* GraphStitchingCheck(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphStitchingCheck", true, graphCallback, commonData, true);
    GraphData* gd = graph->getGraphData();

    //CvFilter filter(new openCVGraph::ImageStatistics("ImageStatistics", *gd, CV_16UC1));
    //graph->AddFilter(filter);

    return graph;
}

GraphManager* GraphImageDir(GraphCommonData * commonData)
{
    // Create a graph
    GraphManager* graph = new GraphManager("GraphImageDir", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    graph->UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap(new CamDefault("CamDefault", *gd));
    graph->AddFilter(cap);

    //CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    //graph->AddFilter(canny);

    return graph;
}



// -----------------------------------------------------------------------------------
// The Temca Class used by Python
// -----------------------------------------------------------------------------------

class Temca
{
public:
    Temca() {
        // Set up logging
        try
        {
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
            std::cout << "Log failed: " << ex.what() << std::endl;
        }

    }

    // Create all graphs 
    bool init(const char * graphType, StatusCallbackType callback)
    {
        bool fOK = true;
        string sGraphType(graphType);
        m_PythonCallback = callback;

        // Create the graphs
        if (sGraphType == "dummy") {
            m_gmCapture = GraphCamXimeaDummy(m_graphCommonData);
        }
        else {
            m_gmCapture = GraphCamXimea(m_graphCommonData);
        }
        m_gmFileWriter = GraphFileWriter(m_graphCommonData);
        m_gmQC = GraphQC(m_graphCommonData);
        m_gmStitchingCheck = GraphStitchingCheck(m_graphCommonData);

        // Create the graphs steps.  
        // Each step runs to completion.  
        // Each graph in a step runs in parallel with other graphs in the step.
        m_StepCapture = new GraphParallelStep("StepCapture", list<GraphManager*> { m_gmCapture });
        m_StepPostCapture = new GraphParallelStep("StepPostCapture", list<GraphManager*> { m_gmFileWriter, m_gmQC, m_gmStitchingCheck });
        // ... could have more steps here

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

        FindTemcaInterfaces();

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
        
        memcpy(image, gd->m_CommonData->m_imCapture.data, size_t( gd->m_CommonData->m_imCapture.size().area() * sizeof (UINT16)));
    }


    CameraInfo getCameraInfo() {
        CameraInfo fi;
        if (m_ITemcaCamera) {
            ITemcaCamera* pCam = dynamic_cast<ITemcaCamera *> (m_ITemcaCamera.get());
            fi = pCam->getCameraInfo();
        }
        return fi;
    }


private:
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
    Processor m_ITemcaCamera = NULL;

    // callback to python
    StatusCallbackInfo m_PythonInfo = { 0 };
    StatusCallbackType m_PythonCallback = NULL;

    void FindTemcaInterfaces()
    {
        for (auto processor : m_gmCapture->GetFilters()) {
            // the next line took me 2 hours to figure out!!!
            if (dynamic_cast<ITemcaCamera *> (processor.get()) != nullptr)
            {
                m_ITemcaCamera = processor;
            }
        }

    }

    bool PythonCallback(int status, int error, const char * errorString) {
        bool fOK = true;
        if (m_PythonCallback) {
            m_PythonInfo.status = status;
            m_PythonInfo.error_code = error;
            strcpy_s(m_PythonInfo.error_string, errorString);
            // Keep going if the python callback returns True
            fOK = ((m_PythonCallback)(&m_PythonInfo) != 0);
        }
        return fOK;
    }

    enum StatusCodes {
        FatalError = -1,
        InitFinished = 0,
        StartNewFrame = 1,
        CaptureFinished = 2,
        ProcessingFinished = 3,
        ShutdownFinished = 4
    };

    // The main capture loop
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

                    if (!(fOK = m_StepCapture->Step())) {
                        s = m_StepCapture->GetName() + " failed Capture Step.";
                        m_Logger->error(s);
                        PythonCallback(FatalError, 0, s.c_str());
                        m_Aborting = true;
                    }
                    else {
                        fOK &= PythonCallback(CaptureFinished, 0, "");

                        if (!(fOK &= m_StepCapture->WaitStepCompletion())) {
                            s = m_StepCapture->GetName() + " Capture WaitStepCompletion.";
                            m_Logger->error(s);
                            PythonCallback(FatalError, 0, s.c_str());
                            m_Aborting = true;
                        }
                        else {
                            // step all of the post capture steps
                            for (auto step : m_StepsPostCapture) {
                                if (!(fOK &= step->Step())) {
                                    s = m_StepCapture->GetName() + " failed to Step.";
                                    m_Logger->error(s);
                                    PythonCallback(FatalError, 0, s.c_str());
                                    m_Aborting = true;
                                    break;
                                }
                                if (fOK) {
                                    if (!(fOK &= step->WaitStepCompletion())) {
                                        s = m_StepCapture->GetName() + " failed WaitStepCompletion.";
                                        m_Logger->error(s);
                                        PythonCallback(FatalError, 0, s.c_str());
                                        m_Aborting = true;
                                        break;
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

Temca * pTemca = NULL;

bool init(const char* graphType, StatusCallbackType callback)
{
    string s = string(graphType);

    if (s == "default" || s == "dummy") {
        pTemca = new Temca();
    }
    else {
        // unknown graph type
        return false;
    }

    bool fOK = true;
    fOK = pTemca->init(graphType, callback);
    pTemca->StartThread();
    return fOK;
}

bool fini()
{
    if (pTemca) {
        pTemca->JoinThread();
    }
    return true;
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

StatusCallbackInfo getStatus() {
    StatusCallbackInfo ci;
    ci.status = 42;
    strncpy_s(ci.error_string, "this is not an error", sizeof(ci.error_string) - 1);
    return ci;
}

FocusInfo getFocus() {
    FocusInfo fi = { 0 };
    fi.score = 42;
    return fi;
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