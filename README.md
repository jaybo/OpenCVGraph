# opencv_graph

Purpose: Create simple, reusable OpenCV 3.0 based image processing components which can be arranged into a graph. Each graph runs on a separate thread from the main application thread. The host application can instantiate multiple graphs simultaneously and each graph can be selected to run on either the CPU or a GPU.  Each graph is hosted by a class called **GraphManager**.  

The basic work item in the graph is a **Filter**.  An arbitrary number of Filters can be added to a graph and are called sequentially by the GraphManager to process the current frame.  The Filter base class provides common functionality used by subclasses such as persisting the state of the Filter, computing timing statistics on the filter performance, and default keyboard processing.  A packet of data, the **GraphData** class is passed between each Filter which contains all state information including the frame(s) to be processed. 

The graph has the following states:  
- Stop - no resources are allocated  
- Pause - resources are allocated, and the graph can be single stepped.  
- Run - continuously process frames  

The **GraphParallelStep** class handles grouping multiple graphs together and running them all in parallel.  

Normally the first component in a graph is a capture Filter.  The base class **CamGeneric** Filter can source images from:  
- A camera  
- A still image  
- A directory containing .tiff, .png, or .jpg images  
- A noise generator  

If using CUDA, the next filter in the graph should be CapturePostProcessing which uploads the capture image to CUDA memory and optionally performs bright / darkfield
corrections.

Each Filter can have associated view window(s) and can hook mouse and keyboard input. The **ZoomView** class implements synchronized viewing and pan operations between consenting filters.


### Example


    #include "OpenCVGraph.h"
    
    int main()
    {
        // Create a graph
        GraphCommonData * commonData = new GraphCommonData();
        GraphManager graph("GraphWebCam", true, graphCallback, commonData);
        GraphData* gd = graph.getGraphData();
    
        // Add an image source (could be camera, single image, directory, noise, movie)
        CvFilter cap1(new CamDefault("CamDefault", *gd));
        graph.AddFilter(cap1);
    
        // load the capture image into CUDA and optionally perform Bright/Dark correction and up-shifting
        CvFilter fCapturePostProcessing(new CapturePostProcessing("CapturePostProcessing", *gd, openCVGraph::CaptureRaw, 640, 480, false, false));
        graph.AddFilter(fCapturePostProcessing);

        CvFilter canny1(new openCVGraph::Canny("Canny1", *gd));
        graph.AddFilter(canny1);
    
        // Start the thread for that graph running
        graph.StartThread();
        graph.GotoState(GraphManager::GraphState::Run);
    
        graph.JoinThread();
    }
 
### Building just OpenCVGraph
Enlist in the following two projects.  Keep them at the same level in the directory tree to avoid needing to change include/lib paths:

- opencv_temca
- opencv_graph    

#### Setting environment variables


    OPENCV_DIR=J:/dev/opencv_temca/3.1.0.Cuda/install
    PATH = %PATH%;J:/dev/opencv_temca/3.1.0.Cuda/install/x64/vc12/bin;j:/dev/opencv_graph/
    PYTHONDEVPATH=C:\WinPython-64bit-2.7.10.2\python-2.7.10.amd64


#### Install Tools

- VS2015 or VS2013 Community Edition or better
- NVidia CUDA 7.5 (https://developer.nvidia.com/cuda-toolkit )
