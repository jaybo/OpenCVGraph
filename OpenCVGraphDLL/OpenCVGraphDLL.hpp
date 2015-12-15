// OpenCVGraphDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Exported.hpp"

using namespace std;
using namespace openCVGraph;

// This function is called at the completion of each loop through a graph to check for 
// user input.
bool graphCallback(GraphManager* graphManager)
{
    // waitKey is required in OpenCV to make graphs display, 
    // so this funtion call is required.

    int key = cv::waitKey(1);
    if (graphManager->AbortOnEscape())
    {
        if (key == 27) // ESCAPE
        {
            graphManager->Abort();
            return false;
        }
    }
    return true;
}

GraphManager* GraphWebCam()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphWebCam", true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter camera(new CamDefault("WebCam", gd, CV_8UC3));
    graph->AddFilter(camera);

    return graph;
}


GraphManager* GraphCamXimea()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphCamXimea", true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter camera(new CamXimea("CamXimea", gd, CV_16UC1));
    graph->AddFilter(camera);

    return graph;
}

GraphManager* GraphFileWriter()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphFileWriter", true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter fileWriter(new FileWriter("FileWriter", gd, CV_16UC1));
    graph->AddFilter(fileWriter);

    return graph;
}

GraphManager* GraphQC()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphQC", true, graphCallback);
    GraphData gd = graph->getGraphData();

    //CvFilter fFocusSobel(new FocusSobel("FocusSobel", gd, CV_16UC1, 512, 150));
    //graph->AddFilter(fFocusSobel);

    //CvFilter fFocusFFT(new FocusFFT("FocusFFT", gd, CV_16UC1, 512, 512));
    //graph->AddFilter(fFocusFFT);

    //CvFilter filter(new openCVGraph::ImageStatistics("ImageStatistics", gd, CV_16UC1));
    //graph->AddFilter(filter);

    return graph;
}

GraphManager* GraphStitchingCheck()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphStitchingCheck", true, graphCallback);
    GraphData gd = graph->getGraphData();

    //CvFilter filter(new openCVGraph::ImageStatistics("ImageStatistics", gd, CV_16UC1));
    //graph->AddFilter(filter);

    return graph;
}

GraphManager* GraphImageDir()
{
    // Create a graph
    GraphManager* graph = new GraphManager("GraphImageDir", true, graphCallback);
    GraphData gd = graph->getGraphData();

    graph->UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap(new CamDefault("CamDefault", gd));
    graph->AddFilter(cap);

    //CvFilter canny(new openCVGraph::Canny("Canny", gd));
    //graph->AddFilter(canny);

    return graph;
}

#ifdef WITH_CUDA
//void GraphXimea()
//{
//    // Create a graph
//    GraphManager graph1("GraphXimea", true, graphCallback);
//    GraphData gd = graph1.getGraphData();
//
//    CvFilter cam(new CamXimea("CamXimea", gd, CV_16UC1, 1024, 1024));
//    graph1.AddFilter(cam);
//
//    //CvFilter faverage(new Average("Average", gd));
//    //graph1.AddFilter(faverage);
//
//    CvFilter fbrightDark(new BrightDarkFieldCorrection("BrightDark", gd));
//    graph1.AddFilter(fbrightDark);
//
//    //CvFilter fpRunningStats(new ImageStatistics("Stats", gd));
//    //graph1.AddFilter(fpRunningStats);
//
//    //CvFilter fFocusSobel(new FocusSobel("FocusSobel", gd, CV_16UC1, 512, 150));
//    //graph1.AddFilter(fFocusSobel);
//
//    //CvFilter fFocusFFT(new FocusFFT("FocusFFT", gd, CV_16UC1, 512, 512));
//    //graph1.AddFilter(fFocusFFT);
//
//    //CvFilter canny(new openCVGraph::Canny("Canny", gd));
//    //graph1.AddFilter(canny);
//
//    //CvFilter fpSimple(new Simple("Simple", gd));
//    //graph1.AddFilter(fpSimple);
//
//    //CvFilter fileWriter(new FileWriter("FileWriter", gd));
//    //graph1.AddFilter(fileWriter);
//
//    // Start the thread for that graph running
//    graph1.StartThread();
//    graph1.GotoState(GraphManager::GraphState::Run);
//
//    graph1.JoinThread();
//}
#endif

class Temca
{
public:
    Temca() {
        // Set up logging
        try
        {
            const char * loggerName = "GraphLogs";
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


   /* void Run()
    {
        bool fOK = true;

        list<GraphManager*> capParallel = { cap };
        GraphParallelStep capStep(capParallel);
        fOK &= capStep.init();

        list<GraphManager*> postCapParallel1 = { can, fw };
        GraphParallelStep postCapStep1(postCapParallel1);
        fOK &= postCapStep1.init();


        list<GraphManager*> postCapParallel2 = { car };
        GraphParallelStep postCapStep2(postCapParallel2);
        fOK &= postCapStep2.init();

        GraphData& gdCap = cap->getGraphData();
        GraphData& gdCan = can->getGraphData();
        GraphData& gdFW = fw->getGraphData();
        GraphData& gdCar = car->getGraphData();

        while (fOK) {
            fOK &= capStep.Step();
            fOK &= capStep.WaitStepCompletion();

            if (!fOK) {
                break;
            }

            gdCan.m_imCapture = gdCap.m_imCapture;
            gdCan.CopyCaptureToRequiredFormats();

            gdFW.m_imCapture = gdCap.m_imCapture;
            gdFW.CopyCaptureToRequiredFormats();

            gdCar.m_imCapture = gdCap.m_imCapture;
            gdCar.CopyCaptureToRequiredFormats();

            fOK &= postCapStep1.Step();
            fOK &= postCapStep2.Step();

            fOK &= postCapStep1.WaitStepCompletion();
            fOK &= postCapStep2.WaitStepCompletion();

        }

        capStep.fini();
        postCapStep1.fini();
        postCapStep2.fini();

        cap->JoinThread();
        can->JoinThread();
        fw->JoinThread();
        car->JoinThread();
    }*/

    // Create all graphs 
    bool init()
    {
        bool fOK = true;

        // Create the graphs
        m_gmCapture = GraphCamXimea();
        m_gmFileWriter = GraphFileWriter();
        m_gmQC = GraphQC();
        m_gmStitchingCheck = GraphStitchingCheck();

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
        m_thread.join();
    }

private:
    // The graphs which  can run simultaneous on separate threads, 
    // and either on GPU or CPU
    GraphManager* m_gmCapture = NULL;
    GraphManager* m_gmFileWriter = NULL;
    GraphManager* m_gmQC = NULL;
    GraphManager* m_gmStitchingCheck = NULL;
    
    // Bundled graphs which step together
    GraphParallelStep* m_StepCapture;
    GraphParallelStep* m_StepPostCapture;

    std::list<GraphParallelStep*> m_Steps;
    std::list<GraphParallelStep*> m_StepsPostCapture;

    bool m_Enabled = true;
    bool m_Aborting = false;

    std::thread m_thread;
    //GraphState m_GraphState;
    bool m_Stepping = false;

    std::mutex m_mtx;
    std::condition_variable m_cv;           // 
    bool m_CompletedStep = false;           // Has the step finished?
    bool m_CompletedRun = false;            // Has the run finished?
    
    int m_LogLevel = spd::level::info;
    std::shared_ptr<spdlog::logger> m_Logger;

    // control interfaces
    ITemcaCamera * m_ITemcaCamera;


    // The main capture loop
    bool ProcessLoop()
    {
        bool fOK = true;

        try {
            while (fOK) {
                // Do the capture step
                if (!(fOK = m_StepCapture->Step())) {
                    m_Logger->error(m_StepCapture->GetName() + " failed Capture Step.");
                }
                else {
                    if (!(fOK = m_StepCapture->WaitStepCompletion())) {
                        m_Logger->error(m_StepCapture->GetName() + " failed Capture WaitStepCompletion.");
                    }
                    else {
                        // copy capture image reference to all graphs
                        auto gd = m_gmCapture->getGraphData();
                        for (auto step : m_StepsPostCapture) {
                            step->NewCaptureImage(gd);
                        }

                        // step all of the post capture steps
                        for (auto step : m_StepsPostCapture) {
                            if (!(fOK = step->Step())) {
                                m_Logger->error(step->GetName() + " failed to Step.");
                                break;
                            }
                            if (fOK) {
                                if (!(fOK = step->WaitStepCompletion())) {
                                    m_Logger->error(step->GetName() + " failed WaitStepCompletion.");
                                    break;
                                }
                            }
                        }
                    }
                }
            }

        }
        catch (exception& ex)
        {
            std::cout << "main processing loop: " << ex.what() << std::endl;
        }
    return fOK;
    }


};

Temca * pTemca = NULL;

bool init()
{
    bool fOK = true;
    pTemca = new Temca();
    fOK &= pTemca->init();
    pTemca->StartThread();
    return true;
}

bool fini()
{
    if (pTemca) {
        pTemca->JoinThread();
    }
    return true;
}



