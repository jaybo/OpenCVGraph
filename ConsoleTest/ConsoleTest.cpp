// Test OpenCVGraph functionality by creating free running graphs
//

#include "stdafx.h"
#include "PerfTests.hpp"

extern "C" int  xiSample();

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
    GraphManager graph("GraphWebCam", true, graphCallback, commonData);
    GraphData* gd = graph.getGraphData();

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd));
    graph.AddFilter(cap1);

    //CvFilter faverage(new Average("Average", *gd));
    //graph.AddFilter(faverage);

    CvFilter fpSimple(new Simple("Simple", *gd));
    graph.AddFilter(fpSimple);

    CvFilter fpRunningStats(new ImageStatistics("Stats", *gd));
    graph.AddFilter(fpRunningStats);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", *gd, StreamIn::CaptureRaw, 512, 512));
    graph.AddFilter(fFocusFFT);

    //CvFilter fFocusSobel(new FocusSobel("FocusSobel", *gd, StreamIn::CaptureRaw, 512, 150));
    //graph.AddFilter(fFocusSobel);

    //CvFilter fFocusLaplace(new FocusLaplace("FocusLaplace", *gd, StreamIn::CaptureRaw, 512, 512));
    //graph.AddFilter(fFocusLaplace);

    CvFilter canny1(new openCVGraph::Canny("Canny1", *gd));
    graph.AddFilter(canny1);

    CvFilter canny2(new openCVGraph::Canny("Canny2", *gd));
    graph.AddFilter(canny2);

    CvFilter cartoon1(new openCVGraph::Cartoon("Cartoon1", *gd));
    graph.AddFilter(cartoon1);

    CvFilter cartoon2(new openCVGraph::Cartoon("Cartoon2", *gd));
    graph.AddFilter(cartoon2);

    // Start the thread for that graph running
    graph.StartThread();
    graph.GotoState(GraphManager::GraphState::Run);

    graph.JoinThread();
}

void GraphImageDir()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph("GraphImageDir", true, graphCallback, commonData);
    GraphData* gd = graph.getGraphData();

    graph.UseCuda(false);

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd, StreamIn::CaptureRaw));
    graph.AddFilter(cap1);

    CvFilter fpSimple(new Simple("Simple", *gd, StreamIn::CaptureRaw));
    graph.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
    graph.AddFilter(fileWriter);

    //CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    //graph.AddFilter(canny);

    // Start the thread for that graph running
    graph.StartThread();
    graph.GotoState(GraphManager::GraphState::Run);

    graph.JoinThread();
}


void GraphCopyOldTEMCAUpshifted()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph("GraphCopyOldTEMCAUpshifted", true, graphCallback, commonData);
    GraphData* gd = graph.getGraphData();

    // Add an image source (could be camera, single image, directory, noise, movie)
    CvFilter cap1(new CamDefault("CamDefault", *gd, StreamIn::CaptureRaw));
    graph.AddFilter(cap1);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
    graph.AddFilter(fileWriter);

    // Start the thread for that graph running
    graph.StartThread();
    graph.GotoState(GraphManager::GraphState::Run);

    graph.JoinThread();
}

#ifdef WITH_CUDA
void GraphXimea()
{
    // Create a graph
    GraphCommonData * commonData = new GraphCommonData();
    GraphManager graph("GraphXimea", true, graphCallback, commonData);
    GraphData* gd = graph.getGraphData();

    CvFilter cam2(new CamXimea("CamXimea", *gd, StreamIn::CaptureRaw, 512, 512));
    graph.AddFilter(cam2);

    //CvFilter faverage(new Average("Average", *gd));
    //graph.AddFilter(faverage);

    CvFilter brightDark(new CapturePostProcessing("CapturePostProcessing", *gd));
    graph.AddFilter(brightDark);

    CvFilter fpQC(new ImageQC("QC", *gd));
    graph.AddFilter(fpQC);

    CvFilter fpRunningStats(new ImageStatistics("Stats", *gd));
    graph.AddFilter(fpRunningStats);

    CvFilter fFocusFFT(new FocusFFT("FocusFFT", *gd, StreamIn::CaptureRaw, 512, 512));
    graph.AddFilter(fFocusFFT);

    CvFilter fFocusSobel(new FocusSobel("FocusSobel", *gd, StreamIn::CaptureRaw, 512, 150));
    graph.AddFilter(fFocusSobel);

    CvFilter fFocusLaplace(new FocusLaplace("FocusLaplace", *gd, StreamIn::CaptureRaw, 512, 512));
    graph.AddFilter(fFocusLaplace);

    CvFilter canny(new openCVGraph::Canny("Canny", *gd));
    graph.AddFilter(canny);

    //CvFilter cartoon1(new openCVGraph::Cartoon("Cartoon1", *gd));
    //graph.AddFilter(cartoon1);

    CvFilter fpSimple(new Simple("Simple", *gd));
    graph.AddFilter(fpSimple);

    CvFilter fileWriter(new FileWriter("FileWriter", *gd, StreamIn::CaptureRaw));
    graph.AddFilter(fileWriter);

    // Start the thread for that graph running
    graph.StartThread();
    graph.GotoState(GraphManager::GraphState::Run);

    graph.JoinThread();
}
#endif



int main()
{
#if false
    // GraphCopyOldTEMCAUpshifted();
    // GraphImageDir();
#endif
    for (int d = 512; d < 5000; d+=512)
        timeMatOps(d);
    timeMatOps(3840);

#ifdef WITH_CUDA
    // xiSample();
    // GraphWebCam();
    // GraphXimea();
#else
     GraphWebCam();
#endif
    return 0;
}

