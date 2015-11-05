// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// TODO: reference additional headers your program requires here

#include <cstring>
#include <sstream>
#include <iostream>
#include <memory>
#include <iomanip>
#include <algorithm>

#include "util.h"

// OpenCV
#include <opencv2/opencv.hpp>

// Boost
#include <boost/filesystem.hpp>


//#define OPENCVGUI_DLL 1
//
//#ifdef OPENCVGUI_DLL // abc.dll source code will define this macro before including this header 
//#define OPENCVGUI_API __declspec( dllexport ) 
//#else 
//#define OPENCVGUI_API __declspec( dllimport ) 
//#endif 
//
//#define OPENCVGUI_DLL
