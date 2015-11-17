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

    if (true) {
        // Add an image source (could be camera, single image, directory, noise, movie)
         CvFilter cap1(new CamDefault("CamDefault", graph1.gd));
         graph1.AddFilter(cap1);

         CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd));
         graph1.AddFilter(canny);

         graph1.UseCuda(false);
    }
    else {
        CvFilter cam2(new CamXimea("CamXimea", graph1.gd));
        graph1.AddFilter(cam2);

        //CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd));
        //graph1.AddFilter(canny);

        //CvFilter fpSimple(new Simple("Simple", graph1.gd));
        //graph1.AddFilter(fpSimple);

        CvFilter fpRunningStats(new ImageStatistics("Stats", graph1.gd));
        graph1.AddFilter(fpRunningStats);
    }


    // Start the thread for that graph running
    graph1.StartThread();
    graph1.GotoState(GraphManager::GraphState::Run);

    graph1.JoinThread();

 
    return 0;
}

