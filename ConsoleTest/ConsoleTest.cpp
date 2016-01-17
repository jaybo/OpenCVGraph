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
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph1("GraphWebCam", true, graphCallback, commonData);
    GraphData* gd = graph1.getGraphData();

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd));
    graph1.AddFilter(cap1);

    //CvFilter faverage(new Average("Average", *gd));
    //graph1.AddFilter(faverage);

    CvFilter fpSimple(new Simple("Simple", *gd));
    graph1.AddFilter(fpSimple);

    CvFilter fpRunningStats(new ImageStatistics("Stats", *gd));
    graph1.AddFilter(fpRunningStats);

    CvFilter canny1(new openCVGraph::Canny("Canny1", *gd));
    graph1.AddFilter(canny1);

    CvFilter canny2(new openCVGraph::Canny("Canny2", *gd));
    graph1.AddFilter(canny2);

    CvFilter cartoon1(new openCVGraph::Cartoon("Cartoon1", *gd));
    graph1.AddFilter(cartoon1);

    CvFilter cartoon2(new openCVGraph::Cartoon("Cartoon2", *gd));
    graph1.AddFilter(cartoon2);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();

}



void GraphImageDir()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph1("GraphImageDir", true, graphCallback, commonData);
    GraphData* gd = graph1.getGraphData();

    graph1.UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd, StreamIn::CaptureRaw));
    graph1.AddFilter(cap1);

    CvFilter fpSimple(new Simple("Simple", *gd, StreamIn::CaptureRaw));
    graph1.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
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
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph1("GraphCopyOldTEMCAUpshifted", true, graphCallback, commonData);
    GraphData* gd = graph1.getGraphData();

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd, StreamIn::CaptureRaw));
    graph1.AddFilter(cap1);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
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
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph1("GraphXimea", true, graphCallback, commonData);
    GraphData* gd = graph1.getGraphData();

    CvFilter cam2(new CamXimea("CamXimea", *gd, StreamIn::CaptureRaw, 512, 512));
    graph1.AddFilter(cam2);

    //CvFilter faverage(new Average("Average", *gd));
    //graph1.AddFilter(faverage);

    CvFilter brightDark(new CapturePostProcessing("CapturePostProcessing", *gd));
    graph1.AddFilter(brightDark);

    CvFilter fpQC(new ImageQC("QC", *gd));
    graph1.AddFilter(fpQC);

    CvFilter fpRunningStats(new ImageStatistics("Stats", *gd));
    graph1.AddFilter(fpRunningStats);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", *gd, StreamIn::CaptureRaw, 512, 512));
    graph1.AddFilter(fFocusFFT);

    //CvFilter fFocusSobel(new FocusSobel("FocusSobel", *gd, StreamIn::CaptureRaw, 512, 150));
    //graph1.AddFilter(fFocusSobel);

    //CvFilter fFocusLaplace(new FocusLaplace("FocusLaplace", *gd, StreamIn::CaptureRaw, 512, 512));
    //graph1.AddFilter(fFocusLaplace);

    CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    graph1.AddFilter(canny);

    //CvFilter fpSimple(new Simple("Simple", *gd));
    //graph1.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
    graph1.AddFilter(fileWriter);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();
}

#endif

int xiSample();

int main()
{
#if false
    // GraphCopyOldTEMCAUpshifted();
    // GraphImageDir();
#endif

#ifdef WITH_CUDA
    //xiSample();
    GraphWebCam();
    //GraphXimea();
#else
    GraphWebCam();
#endif
    return 0;
}

