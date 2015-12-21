#pragma once

#ifndef OPENCV_GRAPH_EXPORTED_H
#define OPENCV_GRAPH_EXPORTED_H

typedef struct tStatusCallbackInfo {
    int status; // 0: init complete, 1: grab complete (move stage), 2: graph complete, -1: error, see error_string
    int error_code; // 0: OK, 
    char error_string[256];
} StatusCallbackInfo;

typedef int (*StatusCallbackType)(StatusCallbackInfo* callbackinfo);
// typedef int (*StatusCallbackType)();

typedef struct tFrameInfo {
    int width;
    int height;
    int format;
    int pixel_depth;
    char camera_id[256];
} FrameInfo;

typedef struct tFocusInfo {
    float score;
    float astigmatism;
    float angle;
} FocusInfo;

typedef struct tROIInfo {
    int gridX;
    int gridY;
} ROIInfo;

extern "C" {
    __declspec(dllexport) bool init(const char* graphType, StatusCallbackType callback);
    __declspec(dllexport) bool fini();

    __declspec(dllexport) void setROI(const ROIInfo * roiInfo);

    __declspec(dllexport) void grabFrame(const char* filename, UINT32 roiX, UINT32 roiY);
    __declspec(dllexport) void getLastFrame(unsigned char * image);

    __declspec(dllexport) tFrameInfo getFrameInfo();
    __declspec(dllexport) tStatusCallbackInfo getStatus();
    __declspec(dllexport) tFocusInfo getFocus();


    //__declspec(dllexport) bool loadConfigFiles(const char cam_file[256], const char vic_file[256]);
    //__declspec(dllexport) void freeArray();
    //__declspec(dllexport) void createBuffer();
    //__declspec(dllexport) void acquireImages(UINT32 nframes, const char dir_str[80]);
    //__declspec(dllexport) UINT32 queueFrame();
    //__declspec(dllexport) void printStatus(UINT32 status);
    //__declspec(dllexport) CHAR *getStatusText(UINT32 status);
    //__declspec(dllexport) void disconnectSapera();
    //__declspec(dllexport) void freeSapera();
    //__declspec(dllexport) void closeSapera();
    //__declspec(dllexport) double GetTimeMs();
    //__declspec(dllexport) UINT32 getParameter(UINT32 prm, UINT32 status);
    //__declspec(dllexport) UINT32 setParameter(UINT32 sprm, UINT32 svalue, UINT32 status);
    //__declspec(dllexport) UINT32 getWidth();
    //__declspec(dllexport) UINT32 getHeight();
    //__declspec(dllexport) UINT32 getFormat();
    //__declspec(dllexport) UINT16 *grabFrame();
    //__declspec(dllexport) UINT32 getBufferDataDepth();
    //__declspec(dllexport) UINT32 getBufferPixelDepth();
    //__declspec(dllexport) UINT32 getBufferType();

    //CORSTATUS status; // Error code
    //CORBUFFER hBuffer;
    //CORBUFFER Buffer;// Buffer handle
    //CORSERVER hSystem; // Sys tem server handle
    //CORSERVER hBoard; // Board server handle
    //CORCAM hCam; // CAM handle
    //CORVIC hVic; // VIC handle
    //CORACQ hAcq; // Acquisition handle
    //CORXFER hXfer; // Transfer handle
    //CORFILE hFile; // File handle
    //UINT32 width;
    //UINT32 height;
    //UINT32 format;
    //FILE pFile;
    //UINT32 imgsize;
    //UINT32 pitch; // width of buffer created
    //UINT32 i, j;
    //UINT32 dim;
    char sText[256];
    //UINT16 *pData;
    //UINT32 pixelDepth;
    //UINT32 value;

}
#endif