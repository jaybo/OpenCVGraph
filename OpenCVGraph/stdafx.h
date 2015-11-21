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
#include "include/dirent.h"
#elif defined __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

#include "OpenCVGraph.h"



//#define openCVGraph_DLL 1
//
//#ifdef openCVGraph_DLL // abc.dll source code will define this macro before including this header 
//#define openCVGraph_API __declspec( dllexport ) 
//#else 
//#define openCVGraph_API __declspec( dllimport ) 
//#endif 
//
//#define openCVGraph_DLL
