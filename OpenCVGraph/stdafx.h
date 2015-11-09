// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#endif

// TODO: reference additional headers your program requires here
#include <cstring>
#include <sstream>
#include <iostream>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <numeric>


// OpenCV
#include <opencv2/opencv.hpp>

// Boost
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

#include "OpenCVGraph.h"

//#define XIMEA_DIR
#ifdef XIMEA_DIR
#include <xiApi.h>
#endif

//#define openCVGraph_DLL 1
//
//#ifdef openCVGraph_DLL // abc.dll source code will define this macro before including this header 
//#define openCVGraph_API __declspec( dllexport ) 
//#else 
//#define openCVGraph_API __declspec( dllimport ) 
//#endif 
//
//#define openCVGraph_DLL
