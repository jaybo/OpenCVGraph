# OpenCVGraph

Purpose: Create simple, reusable OpenCV 3.0 based image processing components which can be arranged into a graph.  The graph runs on a separate thread from the main application thread.  The host application can instantiate multiple graphs simultaneously.  At present the graph is just a linear sequence of processing steps with no branching or forking allowed, other than looping back to the beginning when all processors have completed their work on the current frame.  The graph is hosted by a class called **GraphManager**.

The basic work item in the graph is a **FrameProcessor**.  An arbitrary number of FrameProcessors can be added to a graph and are called sequentially by the GraphManager to process the current frame.  A packet of data is passed between each FrameProcessor which contains all state information including the frame(s) to be processed. The FrameProcessor base class provides common functionality used by subclasses such as persisting the state of the FrameProcessor, measuring the time the filter used to complete its task, and default keyboard processing. 

The graph has the following states:  
- Stop - no resources are allocated  
- Pause - resources are allocated, and the graph can be single stepped.  
- Run - continuously process frames  
    
Normally the first component in a graph is a capture FrameProcessor.  The base class **CamGeneric** FrameProcessor can source images from:  
- A camera  
- A still image  
- A directory containing .tiff, .png, or .jpg images  
- A noise generator  
  
Each FrameProcessor can have associated view window(s) and can hook mouse and keyboard input.

Example
-------

    #include "OpenCVGraph.h"
    
    int main()
    {
        // Create a graph
        GraphManager graph1("Graph1");
        
        // Add a camera source
        std::shared_ptr<CamDefault> fpImage1(new CamDefault("CamDefault", graph1.gd, true));
        graph1.Processors.push_back(fpImage1);
    
        // Add an image statistics processor
        std::shared_ptr<FPRunningStats> fpRunningStats(new FPRunningStats("RunningStats", graph1.gd, true, 512, 512, 1000, 10));
        graph1.Processors.push_back(fpRunningStats);

        // Start the thread for that graph running
        graph1.StartThread();
        graph1.GotoState(GraphManager::GraphState::Run);

        // Wait for the graph to finish
        graph1.JoinThread();
    }
 
