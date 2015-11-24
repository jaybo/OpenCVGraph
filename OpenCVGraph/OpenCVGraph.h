#pragma once

#include <cstring>
#include <sstream>
#include <iostream>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>
#include <list>

// SpdLog
#include "include/spdlog/spdlog.h"


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
#include "Filter.hpp"
#include "GraphManager.h"
#include "Capture/CamDefault.h"

#include "Filters/ImageStatistics.hpp"
#include "Filters/Simple.hpp"
#include "Filters/Canny.hpp"
#include "Filters/FocusSobel.hpp"
#include "Filters/Average.hpp"
#include "Filters/BrightDarkField.hpp"
#include "Filters/Cartoon.hpp"
#include "Filters/FileWriter.hpp"

// Camera specific includes
#include "Capture/CameraSDKs/Ximea/API/xiApi.h"
#include "Capture/CamXimea.h"