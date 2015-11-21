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

int main()
{
    if (true) {
        // Create a graph
        GraphManager graph1("GraphWebCam", CV_8UC3, true,  graphCallback);

        graph1.UseCuda(false);

        // Add an image source (could be camera, single image, directory, noise, movie)
         CvFilter cap1(new CamDefault("CamDefault", graph1.gd));
         graph1.AddFilter(cap1);

         CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd));
         graph1.AddFilter(canny);

         // Start the thread for that graph running
         graph1.StartThread();
         graph1.GotoState(GraphManager::GraphState::Run);

         graph1.JoinThread();
    }
    else if (true) {
        // Create a graph
        GraphManager graph1("GraphImageDir", CV_16UC1, true,  graphCallback);

        graph1.UseCuda(false);

        // Add an image source (could be camera, single image, directory, noise, movie)
        CvFilter cap1(new CamDefault("CamDefault", graph1.gd));
        graph1.AddFilter(cap1);

        //CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd));
        //graph1.AddFilter(canny);

        // Start the thread for that graph running
        graph1.StartThread();
        graph1.GotoState(GraphManager::GraphState::Run);

        graph1.JoinThread();

    }

    else if (true) {
        // Create a graph
        GraphManager graph1("GraphXimea", CV_16UC1, true, graphCallback);

        CvFilter cam2(new CamXimea("CamXimea", graph1.gd));
        graph1.AddFilter(cam2);

        CvFilter brightDark(new BrightDarkFieldCorrection("BrightDark", graph1.gd));
        graph1.AddFilter(brightDark);

        //CvFilter faverage(new Average("Average", graph1.gd));
        //graph1.AddFilter(faverage);

        //CvFilter fpRunningStats(new ImageStatistics("Stats", graph1.gd));
        //graph1.AddFilter(fpRunningStats);

        //CvFilter fFocusSobel(new FocusSobel("FocusSobel", graph1.gd, 512, 150));
        //graph1.AddFilter(fFocusSobel);

        //CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd));
        //graph1.AddFilter(canny);

        //CvFilter fpSimple(new Simple("Simple", graph1.gd));
        //graph1.AddFilter(fpSimple);

        // Start the thread for that graph running
        graph1.StartThread();
        graph1.GotoState(GraphManager::GraphState::Run);

        graph1.JoinThread();
    }



 
    return 0;
}

