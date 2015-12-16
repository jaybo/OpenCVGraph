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

void GraphWebCam()
{
    // Create a graph
    GraphManager graph1("GraphWebCam", true, graphCallback);
    GraphData* gd = graph1.getGraphData();

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd));
    graph1.AddFilter(cap1);

    //CvFilter faverage(new Average("Average", gd));
    //graph1.AddFilter(faverage);

    //CvFilter fpSimple(new Simple("Simple", gd, CV_16UC1));
    //graph1.AddFilter(fpSimple);

    CvFilter canny1(new openCVGraph::Canny("Canny1", *gd));
    graph1.AddFilter(canny1);

    CvFilter canny2(new openCVGraph::Canny("Canny2", *gd));
    graph1.AddFilter(canny2);

    //CvFilter cartoon(new openCVGraph::Cartoon("Cartoon", gd));
    //graph1.AddFilter(cartoon);

    //CvFilter cartoon(new openCVGraph::Cartoon("Cartoon", gd));
    //graph1.AddFilter(cartoon);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();

}



void GraphImageDir()
{
    // Create a graph
    GraphManager graph1("GraphImageDir", true, graphCallback);
    GraphData* gd = graph1.getGraphData();

    graph1.UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd, CV_16UC1));
    graph1.AddFilter(cap1);

    CvFilter fpSimple(new Simple("Simple", *gd, CV_16UC1));
    graph1.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, CV_16UC1));
    graph1.AddFilter(fileWriter);

    //CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    //graph1.AddFilter(canny);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();

}


void GraphCopyOldTEMCAUpshifted()
{
    // Create a graph
    GraphManager graph1("GraphCopyOldTEMCAUpshifted", true, graphCallback);
    GraphData* gd = graph1.getGraphData();

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd, CV_16UC1));
    graph1.AddFilter(cap1);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, CV_16UC1));
    graph1.AddFilter(fileWriter);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();
}

#ifdef WITH_CUDA
void GraphXimea()
{
    // Create a graph
    GraphManager graph1("GraphXimea", true, graphCallback);
    GraphData* gd = graph1.getGraphData();

    CvFilter cam2(new CamXimea("CamXimea", *gd, CV_16UC1, 1024, 1024));
    graph1.AddFilter(cam2);

    CvFilter faverage(new Average("Average", *gd));
    graph1.AddFilter(faverage);

    CvFilter brightDark(new BrightDarkFieldCorrection("BrightDark", *gd));
    graph1.AddFilter(brightDark);

    CvFilter fpRunningStats(new ImageStatistics("Stats", *gd));
    graph1.AddFilter(fpRunningStats);

    CvFilter fFocusSobel(new FocusSobel("FocusSobel", *gd, CV_16UC1, 512, 150));
    graph1.AddFilter(fFocusSobel);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", *gd, CV_16UC1, 512, 512));
    graph1.AddFilter(fFocusFFT);

    CvFilter fFocusLaplace(new FocusLaplace("FocusLaplace", *gd, CV_16UC1, 512, 512));
    graph1.AddFilter(fFocusLaplace);

    CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    graph1.AddFilter(canny);

    //CvFilter fpSimple(new Simple("Simple", *gd));
    //graph1.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, CV_16UC1));
    graph1.AddFilter(fileWriter);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();
}

#endif


int main()
{
    // GraphCopyOldTEMCAUpshifted();

    // GraphWebCam();
    //GraphImageDir();
#ifdef WITH_CUDA
    GraphXimea();
#endif
    return 0;
}

