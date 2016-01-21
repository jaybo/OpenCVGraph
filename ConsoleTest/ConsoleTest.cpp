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

int xiSample();

// Jay's Lenovo Desktop
//Mat dimensions: 3840x3840 CV_16UC1
//
//m1 = m1 * 16                   : 0.0266
//mm[x][y] <<= 4                 : 0.0347
//gm1.upload(m1)                 : 0.0232
//gm1.download(m1)               : 0.0220
//cuda::add(gm1, gm2, gm1)       : 0.0005
//cuda::lshift(gm1, 4, gm1)      : 0.0006
//cuda::multiply (gm1, 16, gm1)  : 0.0004
//cuda::divide(gm1, 16, gm2)     : 0.0006

// Jay's Lenovo Carbon X1 laptop
//Mat dimensions: 3840x3840 CV_16UC1
//
//m1 = m1 * 16                   : 0.0254
//mm[x][y] <<= 4                 : 0.0347

void timeMatOps()
{
    int nLoopCount;
    const int d = 3840;

    auto mm = new uint16_t[d][d]();

    Mat m1(d, d, CV_16UC1);
    Mat m2(d, d, CV_16UC1);
    cv::randu(m1, Scalar::all(0), Scalar::all(65535));
    cv::randu(m2, Scalar::all(0), Scalar::all(65535));
#ifdef WITH_CUDA
    GpuMat gm1(m1);
    GpuMat gm2(m1);
#endif
    auto timer = Timer();
    nLoopCount = 100;

    cout << "Mat dimensions: " << d << "x" << d << " CV_16UC1" << std::endl << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
         // 27 mS
         m1 = m1 * 16;
    }
    cout << "m1 = m1 * 16                   : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 34mS
        for (int x = 0; x < d; x++) {
            for (int y = 0; y < d; y++) {
                mm[x][y] <<= 4;
            }
        }
    }
    cout << "mm[x][y] <<= 4                 : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

#ifdef WITH_CUDA
    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 28mS
        gm1.upload(m1);
    }
    cout << "gm1.upload(m1)                 : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 22mS
        gm1.download(m1);
    }
    cout << "gm1.download(m1)               : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    nLoopCount = 1000;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        cuda::add(gm1, gm2, gm1);
    }
    cout << "cuda::add(gm1, gm2, gm1)       : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 0.9mS
        cuda::lshift(gm1, 4, gm1);
    }
    cout << "cuda::lshift(gm1, 4, gm1)      : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 0.3mS
        cuda::multiply (gm1, 16, gm1);
    }
    cout << "cuda::multiply (gm1, 16, gm1)  : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;

    timer.reset();
    for (int i = 0; i < nLoopCount; i++) {
        // 0.4mS
        cuda::divide(gm1, 16, gm2);
    }
    cout << "cuda::divide(gm1, 16, gm2)     : " << fixed << setprecision(4) << (timer.elapsed() / nLoopCount) << std::endl;
#endif
}


int main()
{
#if false
    // GraphCopyOldTEMCAUpshifted();
    // GraphImageDir();
#endif
    // timeMatOps();

#ifdef WITH_CUDA
    // xiSample();
    // GraphWebCam();
    GraphXimea();
#else
     GraphWebCam();
#endif
    return 0;
}

