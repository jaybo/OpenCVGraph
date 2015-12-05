// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace openCVGraph;

bool graphCallback(GraphManager* graphManager) {
    //cin.sync_with_stdio(false);
    //auto count = std::cin.rdbuf()->in_avail();
    //if (count > 0) {
    if (_kbhit()) {
        int key = _getch();
        if (key == 'r' || key == 'R') {
            graphManager->GotoState(GraphManager::GraphState::Run);
        }
        else if (key == 's' || key == 'S') {
            graphManager->GotoState(GraphManager::GraphState::Stop);
        }
        else if (key == 'p' || key == 'P') {
            graphManager->GotoState(GraphManager::GraphState::Pause);
        }
        else if (key == ' ') {
            graphManager->Step();
        }
        else if (key == 27)
        {
            return false;
        }
    }
    return true;
}

GraphManager* GraphWebCam()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphWebCam", CV_8UC3, true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter camera (new CamDefault("WebCam", gd));
    graph->AddFilter(camera);

    return graph;
}


GraphManager* GraphCamXimea()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphCamXimea2", CV_16UC1, true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter camera(new CamXimea("CamXimea", gd));
    graph->AddFilter(camera);

    return graph;
}

GraphManager* GraphFileWriter()
{
    // Create a graph
    GraphManager *graph = new GraphManager ("GraphFileWriter", CV_8UC3, true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter fileWriterTIFF(new FileWriterTIFF("FileWriterTIFF", gd));
    graph->AddFilter(fileWriterTIFF);

    return graph;
} 

GraphManager* GraphCanny()
{
    // Create a graph
    GraphManager *graph = new GraphManager("GraphCanny", CV_8UC3, true, graphCallback);
    GraphData gd = graph->getGraphData();

    CvFilter canny (new openCVGraph::Canny("Canny", gd));
    graph->AddFilter(canny);
    graph->UseCuda(false);

    return graph;
}


void GraphImageDir()
{
    // Create a graph
    GraphManager graph1("GraphImageDir", CV_16UC1, true, graphCallback);
    GraphData gd = graph1.getGraphData();

    graph1.UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", gd));
    graph1.AddFilter(cap1);

    //CvFilter canny(new openCVGraph::Canny("Canny", gd));
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
    GraphManager graph1("GraphXimea", CV_16UC1, true, graphCallback);
    GraphData gd = graph1.getGraphData();

    CvFilter cam2(new CamXimea("CamXimea", gd, 1024, 1024));
    graph1.AddFilter(cam2);

    CvFilter faverage(new Average("Average", gd));
    graph1.AddFilter(faverage);

    CvFilter brightDark(new BrightDarkFieldCorrection("BrightDark", gd));
    graph1.AddFilter(brightDark);

    CvFilter fpRunningStats(new ImageStatistics("Stats", gd));
    graph1.AddFilter(fpRunningStats);

    CvFilter fFocusSobel(new FocusSobel("FocusSobel", gd, 512, 150));
    graph1.AddFilter(fFocusSobel);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", gd, 512, 512));
    graph1.AddFilter(fFocusFFT);

    //CvFilter canny(new openCVGraph::Canny("Canny", gd));
    //graph1.AddFilter(canny);

    //CvFilter fpSimple(new Simple("Simple", gd));
    //graph1.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", gd));
    graph1.AddFilter(fileWriter);

    CvFilter fileWriterTIFF(new FileWriterTIFF("FileWriterTIFF", gd));
    graph1.AddFilter(fileWriterTIFF);

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
    GraphManager *can = GraphCanny();

public:
    void Run()
    {
        cap->StartThread();
        can->StartThread();

        GraphData& gdCap = cap->getGraphData();
        GraphData& gdCan = can->getGraphData();

        //char* b = new char[80];
        //char* c = "abcdefghijk";

        //gdCap.SetProperty("foo", b);
        //gdCap.SetProperty("f33", c);
        //
        //char * d = (char *) gdCap.GetProperty("f33");


        cap->GotoState(GraphManager::GraphState::Pause);
        can->GotoState(GraphManager::GraphState::Pause);
        
        std::mutex& capMtx = cap->getWaitMutex();
        std::condition_variable& capCV = cap->getConditionalVariable();
        std::mutex& canMtx = can->getWaitMutex();
        std::condition_variable& canCV = can->getConditionalVariable();

        bool fOK = true;
        while (fOK) {
            fOK &= cap->Step();
            {
                std::unique_lock<std::mutex> lk(capMtx);
                while (!cap->CompletedStep())
                    capCV.wait(lk);
            }

            gdCan.m_imCapture = gdCap.m_imCapture;
            gdCan.CopyCaptureToRequiredFormats();
            fOK &= can->Step();
            {
                std::unique_lock<std::mutex> lk(capMtx);
                while (!can->CompletedStep())
                    canCV.wait(lk);
            }
        }

        cap->JoinThread();
        can->JoinThread();
    }
};


int main()
{

    Temca t;
    t.Run();

    return 0;
}

