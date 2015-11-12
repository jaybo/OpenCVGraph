#pragma once

#include <cstring>
#include <sstream>
#include <iostream>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <chrono>


#include <list>
#include <boost/any.hpp>


// Boost
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

// OpenCV
#include <opencv2/opencv.hpp>
#include "opencv2/core.hpp"
#include "opencv2/core/cuda.hpp"
#include <opencv2/cudaimgproc.hpp>
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudafilters.hpp"
#include "opencv2/cudawarping.hpp"

// Local
#include "util.h"
#include "GraphData.h"
#include "ZoomView.h"
#include "Filter.h"
#include "GraphManager.h"
#include "Capture/CamDefault.h"

#include "Filters/FPRunningStats.h"
#include "Filters/Simple.hpp"
#include "Filters/CudaHistogram.hpp"

// Camera specific includes
#include "Capture/CameraSDKs/Ximea/API/xiApi.h"
#include "Capture/CamXimea.h"