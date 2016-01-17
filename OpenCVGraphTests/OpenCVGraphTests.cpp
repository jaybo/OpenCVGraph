// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

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
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager *graph = new GraphManager("GraphWebCam", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter camera(new CamDefault("WebCam", *gd, StreamIn::CaptureRaw));
    graph->AddFilter(camera);

    return graph;
}


GraphManager* GraphCamXimea()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager *graph = new GraphManager("GraphCamXimea2", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter camera(new CamXimea("CamXimea", *gd, StreamIn::CaptureRaw));
    graph->AddFilter(camera);

    return graph;
}

GraphManager* GraphFileWriter()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager *graph = new GraphManager("GraphFileWriter", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
    graph->AddFilter(fileWriter);

    return graph;
}

GraphManager* GraphCanny()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager *graph = new GraphManager("GraphCanny", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter canny(new openCVGraph::Canny("Canny", *gd, StreamIn::CaptureRaw));
    graph->AddFilter(canny);
    //graph->UseCuda(false);

    return graph;
}

GraphManager* GraphCartoon()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager *graph = new GraphManager("GraphCartoon", true, graphCallback, commonData);
    GraphData* gd = graph->getGraphData();

    CvFilter cartoon(new openCVGraph::Cartoon("Cartoon", *gd, StreamIn::CaptureRaw));
    graph->AddFilter(cartoon);
    //graph->UseCuda(false);

    return graph;
}



void GraphImageDir()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph1("GraphImageDir", true, graphCallback, commonData);
    GraphData* gd = graph1.getGraphData();

    graph1.UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd));
    graph1.AddFilter(cap1);

    //CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    //graph1.AddFilter(canny);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();

}

#ifdef WITH_CUDA
void GraphXimea()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph1("GraphXimea", true, graphCallback, commonData);
    GraphData* gd = graph1.getGraphData();

    CvFilter cam2(new CamXimea("CamXimea", *gd, StreamIn::CaptureRaw, 1024, 1024));
    graph1.AddFilter(cam2);

    CvFilter faverage(new Average("Average", *gd));
    graph1.AddFilter(faverage);

    CvFilter brightDark(new CapturePostProcessing("CapturePostProcessing", *gd));
    graph1.AddFilter(brightDark);

    CvFilter fpRunningStats(new ImageStatistics("Stats", *gd));
    graph1.AddFilter(fpRunningStats);

    CvFilter fFocusSobel(new FocusSobel("FocusSobel", *gd, StreamIn::CaptureRaw, 512, 150));
    graph1.AddFilter(fFocusSobel);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", *gd, StreamIn::CaptureRaw, 512, 512));
    graph1.AddFilter(fFocusFFT);

    //CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    //graph1.AddFilter(canny);

    //CvFilter fpSimple(new Simple("Simple", *gd));
    //graph1.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd));
    graph1.AddFilter(fileWriter);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();
}
#endif

class Temca
{
private:
    GraphManager* cap = GraphCamXimea();
    //GraphManager* cap = GraphWebCam();
    GraphManager *can = GraphCanny();
    GraphManager *fw = GraphFileWriter();
    GraphManager *car = GraphCartoon();

public:
    void Run()
    {
        bool fOK = true;

        list<GraphManager*> capParallel = { cap };
        GraphParallelStep capStep("StepCapture", capParallel);
        fOK &= capStep.init();

        list<GraphManager*> postCapParallel1 = { can, fw };
        GraphParallelStep postCapStep1("StepPost1", postCapParallel1);
        fOK &= postCapStep1.init();


        list<GraphManager*> postCapParallel2 = { car};
        GraphParallelStep postCapStep2("StepPost2", postCapParallel2);
        fOK &= postCapStep2.init();

        GraphData* gdCap = cap->getGraphData();
        GraphData* gdCan = can->getGraphData();
        GraphData* gdFW = fw->getGraphData();
        GraphData* gdCar = car->getGraphData();

        while (fOK) {
            fOK &= capStep.Step();
            fOK &= capStep.WaitStepCompletion();

            if (!fOK) {
                break;
            }

            gdCan->m_CommonData->m_imCapture = gdCap->m_CommonData->m_imCapture;
            gdCan->UploadCaptureToCuda();

            gdFW->m_CommonData->m_imCapture = gdCap->m_CommonData->m_imCapture;
            gdFW->UploadCaptureToCuda();

            gdCar->m_CommonData->m_imCapture = gdCap->m_CommonData->m_imCapture;
            gdCar->UploadCaptureToCuda();

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
    }
};


int main()
{
    Temca t;
    t.Run();

    return 0;
}

