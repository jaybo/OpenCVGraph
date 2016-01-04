# OpenCVGraph

Purpose: Create simple, reusable OpenCV 3.0 based image processing components which can be arranged into a graph.  The graph runs on a separate thread from the main application thread.  The host application can instantiate multiple graphs simultaneously.  At present the graph is just a linear sequence of processing steps with no branching or forking allowed, other than looping back to the beginning when all processors have completed their work on the current frame.  The graph is hosted by a class called **GraphManager**.

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
  
Each Filter can have associated view window(s) and can hook mouse and keyboard input. The **ZoomView** class implements synchronized viewing and pan operations between consenting filters.


### Example


    #include "OpenCVGraph.h"
    
    int main()
    {
        // Create a graph
        GraphCommonData * commonData = new GraphCommonData();
        GraphManager graph1("GraphWebCam", true, graphCallback, commonData);
        GraphData* gd = graph1.getGraphData();
    
        // Add an image source (could be camera, single image, directory, noise, movie)
        CvFilter cap1(new CamDefault("CamDefault", *gd));
        graph1.AddFilter(cap1);
    
        CvFilter canny1(new openCVGraph::Canny("Canny1", *gd));
        graph1.AddFilter(canny1);
    
        // Start the thread for that graph running
        graph1.StartThread();
        graph1.GotoState(GraphManager::GraphState::Run);
    
        graph1.JoinThread();
    }
 
### Building OpenCV with CUDA and Ximea support
The stock distribution of OpenCV does not include Cuda support or Ximea support so we need to create a custom build from the OpenCV source tree.


#### Install Tools

- VS2013 Community Edition (required for building CUDA in OpenCV)  It is likely VS2015 will be supported when CUDA 8.0 is released.
- CMake
- NVidia CUDA 7.5

#### Download OpenCV

- Use at least (tag) 3.1.0 of OpenCV. This is the first version which supports Ximea in 16bpp mode.
- From GitHub, download or fork the following projects:

        Itseez/opencv (hereafter "opencv")
        Itseez/opencv_contrib (hereafter "opencv_contrib")

#### Use CMake GUI to create solution files

CMake is used to create the .sln and .proj files for VisualStudio.
-  "Where is the source code" select the "opencv" directory 
-  "Where to build the binaries" select a new directory (not within opencv) hereafter "dest_dir"
-  Click "Configure"
-  Check "WITH_XIMEA"
-  (optionally) Check "BUILD_EXAMPLES" if you want all the examples to be pre-built.
-  Find "OPENCV_EXTRA_MODULES_PATH" and set the path to "opencv_contrib" + "/modules" as in "J:/dev/opencv_contrib/modules"
-  Click "Generate".  When asked what compiler to use, select "Visual Studio 2013 x64".  VS2015 isn't supported yet to generate the CUDA code, although it can be used for everything else other than actually building OpenCV.

#### Build OpenCV with Visual Studio 2013

- Open OPENCV.sln in "dest_dir"
- Build Debug (this will take a few hours)
- Build Release (this will take a few hours)
- Right click on the "INSTALL" project and select "Project Only -> Build Only INSTALL"
- At this point you should have a complete build

#### Setting environment variables

OPENCVEXE_DIR=J:/dev/OpenCVBuilds/3.1.0.Cuda/install/x64/vc12/bin
OPENCVLIB_DIR=J:/dev/OpenCVBuilds/3.1.0.Cuda/install/x64/vc12/lib
OPENCV_DIR=J:/dev/OpenCVBuilds/3.1.0.Cuda/install
PATH = %PATH%;J:/dev/OpenCVBuilds/3.1.0.Cuda/install/x64/vc12/bin
