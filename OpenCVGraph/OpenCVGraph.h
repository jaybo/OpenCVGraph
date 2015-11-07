#pragma once

#include <stdio.h>
#include <tchar.h>
#include <memory>
#include <cstring>
#include <iostream>

// Boost
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

// OpenCV
#include <opencv2/opencv.hpp>

// Local
#include "util.h"
#include "GraphData.h"
#include "ZoomView.h"
#include "FrameProcessor.h"
#include "GraphManager.h"
#include "Capture/CamDefault.h"
#include "Capture/CamXimea.h"
#include "FrameProcessors/FPRunningStats.h"
