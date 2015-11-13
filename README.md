# OpenCVGraph

Purpose: Create simple, reusable OpenCV 3.0 based image processing components which can be arranged into a graph.  The graph runs on a separate thread from the main application thread.  The host application can instantiate multiple graphs simultaneously.  At present the graph is just a linear sequence of processing steps with no branching or forking allowed, other than looping back to the beginning when all processors have completed their work on the current frame.  The graph is hosted by a class called **GraphManager**.

The basic work item in the graph is a **Filter**.  An arbitrary number of Filters can be added to a graph and are called sequentially by the GraphManager to process the current frame.  The Filter base class provides common functionality used by subclasses such as persisting the state of the Filter, computing timing statistics on the filter performance, and default keyboard processing.  A packet of data, the **GraphData** class is passed between each Filter which contains all state information including the frame(s) to be processed. 

The graph has the following states:  
- Stop - no resources are allocated  
- Pause - resources are allocated, and the graph can be single stepped.  
- Run - continuously process frames  
    
Normally the first component in a graph is a capture Filter.  The base class **CamGeneric** Filter can source images from:  
- A camera  
- A still image  
- A directory containing .tiff, .png, or .jpg images  
- A noise generator  
  
Each Filter can have associated view window(s) and can hook mouse and keyboard input. The **ZoomView** class implements synchronized viewing and pan operations between consenting filters.


Example
-------

    #include "OpenCVGraph.h"
    
    int main()
    {
        // Create a graph
        GraphManager graph1("Graph1");
        
        // Add a camera source
        CvFilter fpImage1(new CamXimea("CamXimea", graph1.gd, true));
        graph1.AddFilter(fpImage1);
    
        // Add an image statistics processor
        CvFilter runningStats(new ImageStatistics("RunningStats", graph1.gd, true));
        graph1.AddFilter(runningStats);
        
        // Add a Canny edge detection filter
        CvFilter canny(new openCVGraph::Canny("Canny", graph1.gd, true));
        graph1.AddFilter(canny);

        // Start the thread for that graph running
        graph1.StartThread();
        graph1.GotoState(GraphManager::GraphState::Run);

        // Wait for the graph to finish
        graph1.JoinThread();
    }
 
