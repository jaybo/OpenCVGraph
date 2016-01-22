// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
// Windows Header Files:
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <conio.h>
#include <direct.h>
#include "../OpenCVGraph/include/dirent.h"
#elif defined __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

// TODO: reference additional headers your program requires here
#include <opencv2/opencv.hpp>
#include "../OpenCVGraph/OpenCVGraph.h"


