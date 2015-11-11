// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\OpenCVGraph\OpenCVGraph.h"

using namespace std;
using namespace openCVGraph;

int main()
{

    // Create a graph
    GraphManager graph1("Graph1");

    // Add an image source (could be camera, single image, directory, noise, movie)
    // CvFilter fpImage1(new CamDefault("CamDefault", graph1.gd, true));
    CvFilter fpImage1(new CamXimea("CamXimea", graph1.gd, true));
    graph1.AddFilter(fpImage1);

    CvFilter fpHistogram(new CudaHistogram("CudaHistogram", graph1.gd, true));
    graph1.AddFilter(fpHistogram);

    
    // Add processors
    //CvFilter fpSimple(new Simple("Simple", graph1.gd, true));
    //graph1.AddFilter(fpSimple);

    CvFilter fpRunningStats(new FPRunningStats("RunningStats", graph1.gd, true));
    graph1.AddFilter(fpRunningStats);



    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);


    //GraphManager graph2("Graph2");
    //CvFilter fpImage2 (new CamDefault("CamDefault", graph2.gd, true));
    //graph2.AddFilter(fpImage2);
    //graph2.StartThread();
    //graph2.GotoState(GraphManager::GraphState::Run);


    graph1.JoinThread();
    //graph2.JoinThread();

    /*
    //Mat a(200, 200, CV_16U);
    //Mat b(200, 200, CV_8U);
    //randu(b, Scalar::all(0), Scalar::all(255));
    //randu(a, Scalar::all(0), Scalar::all(64000));

    
    ZoomView view1("viewA", a, 1024, 1024, 100, 100);
    ZoomView view2("viewB", b, 300, 300, 400,400);

    bool fOK = true;
    while (fOK) {
        randu(a, Scalar::all(0), Scalar::all(64000));
        view1.UpdateView();
        view2.UpdateView();
    }*/


    return 0;
}

