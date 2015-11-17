// ConsoleTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/log/common.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>


using namespace std;
using namespace openCVGraph;

bool graphCallback(GraphManager* graphManager) {
    cin.sync_with_stdio(false);
    auto count = std::cin.rdbuf()->in_avail();
    if (count > 0) {
        auto key = std::getchar();
        if (key == 'r' || key == 'R') {
            graphManager->GotoState(GraphManager::GraphState::Run);
        }
        else if (key == 's' || key == 'S') {
            graphManager->GotoState(GraphManager::GraphState::Stop);
        }
        else if (key == 'P' || key == 'P') {
            graphManager->GotoState(GraphManager::GraphState::Pause);
        }
        else if (key == ' ') {
            graphManager->Step();
        }

    }

    return true;
}

int main()
{
    // boost::log::sources::severity_logger< severity_level > lg;

    // Create a graph
    GraphManager graph1("Graph1", true, graphCallback);

    if (false) {
        // Add an image source (could be camera, single image, directory, noise, movie)
         CvFilter fpImage1(new CamDefault("CamDefault", graph1.gd, true));
         graph1.AddFilter(fpImage1);

         CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd, true));
         graph1.AddFilter(canny);


    }
    else {
        CvFilter fpImage1(new CamXimea("CamXimea", graph1.gd, true));
        graph1.AddFilter(fpImage1);

        //CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd, true));
        //graph1.AddFilter(canny);

        //CvFilter fpSimple(new Simple("Simple", graph1.gd, true));
        //graph1.AddFilter(fpSimple);

        CvFilter fpRunningStats(new ImageStatistics("Stats", graph1.gd, true));
        graph1.AddFilter(fpRunningStats);
    }






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

