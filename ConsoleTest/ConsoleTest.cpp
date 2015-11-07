// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Boost
#include <boost/thread.hpp>

#include "..\OpenCVGraph\FrameProcessor.h"
#include "..\OpenCVGraph\GraphManager.h"
#include "..\OpenCVGraph\ZoomView.h"
#include "..\OpenCVGraph\Capture\CamDefault.h"
#include "..\OpenCVGraph\Capture\CamXimea.h"
#include "..\OpenCVGraph\FrameProcessors\FPRunningStats.h"
#include <boost/filesystem.hpp>

using namespace cv;
using namespace std;
using namespace openCVGui;
namespace fs = ::boost::filesystem;


int main()
{
    // Create a graph
    GraphManager graph1("Graph1");

    // Add an image source (could be camera, single image, directory, noise, movie)
   std::shared_ptr<CamDefault> fpImage1(new CamDefault("CamDefault", graph1.gd, true));
 //   std::shared_ptr<CamXimea> fpImage1(new CamXimea("CamXimea", graph1.gd, true));
    graph1.Processors.push_back(fpImage1);
    
    // Add processors
    std::shared_ptr<FPRunningStats> fpRunningStats(new FPRunningStats("RunningStats", graph1.gd, true, 512, 512, 1000, 10));
    graph1.Processors.push_back(fpRunningStats);

    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);




    //GraphManager graph2("Graph2");
    //std::shared_ptr<FPImageSource> fpImage2 (new FPImageSource("Image2", graph2.gd, true));
    //graph2.Processors.push_back(fpImage2);
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
        view1.ProcessEvents();
        view2.ProcessEvents();
    }*/


    return 0;
}

