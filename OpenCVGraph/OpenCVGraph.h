#pragma once

// Comment out the following to disable all CUDA processing
#define WITH_CUDA

#include <cstring>
#include <sstream>
#include <iostream>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>
#include <mutex>              
#include <list>
#include <map>
#include <condition_variable> 

// SpdLog
#include "include/spdlog/spdlog.h"

// OpenCV
#include <opencv2/opencv.hpp>
#include "opencv2/core.hpp"
#include "opencv2/core/cuda.hpp"

#ifdef WITH_CUDA
#include <opencv2/cudaimgproc.hpp>
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudafilters.hpp"
#include "opencv2/cudawarping.hpp"
#endif

// Local
#include "timer.hpp"
#include "util.h"
#include "GraphData.h"
#include "ZoomView.hpp"
#include "Filter.hpp"
#include "GraphManager.hpp"
#include "GraphParallelStep.hpp"

// Interfaces
#include "ITemcaCamera.hpp"
#include "ITemcaQC.hpp"
#include "ITemcaFocus.hpp"
#include "ITemcaCapturePostProcessing.hpp"

// Filters
#include "Filters/Simple.hpp"
#include "Filters/Delay.hpp"
#include "Capture/CamDefault.hpp"
#include "Filters/Canny.hpp"
#include "Filters/Average.hpp"
#include "Filters/Cartoon.hpp"
#include "Filters/ImageQC.hpp"
#include "Filters/ImageStatistics.hpp"
#include "Filters/FileWriter.hpp"

//#ifdef WITH_CUDA
#include "Filters/FocusSobel.hpp"
#include "Filters/FocusLaplace.hpp"
#include "Filters/FocusFFT.hpp"
#include "Filters/CapturePostProcessing.hpp"
//#endif

// Camera specific includes
#include "Capture/CameraSDKs/Ximea/API/xiApi.h"
#include "Capture/CamXimea.hpp"
#include "Capture/CamXimeaOpenCV.hpp"
