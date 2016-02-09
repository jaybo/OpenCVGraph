#pragma once

typedef struct tStatusCallbackInfo {
    int status;         // -1: fatal error, 0: init complete, 1: grab complete (move stage), 2: graph complete, -1: error, see error_string
    int info_code;      //  
    char error_string[256];
} StatusCallbackInfo;

typedef int (*StatusCallbackType)(StatusCallbackInfo* callbackinfo);

typedef struct tROIInfo {
    int gridX;
    int gridY;
} ROIInfo;

extern "C" {
    __declspec(dllexport) bool temca_open(bool fDummyCamera, StatusCallbackType callback);
    __declspec(dllexport) bool temca_close();

    __declspec(dllexport) bool setMode(const char* graphType);

    __declspec(dllexport) tCameraInfo getCameraInfo();
    __declspec(dllexport) tFocusInfo getFocusInfo();
    __declspec(dllexport) tQCInfo getQCInfo();

    __declspec(dllexport) void grabFrame(const char* filename, UINT32 roiX, UINT32 roiY);
    __declspec(dllexport) void getLastFrame(UINT16 * image);
    __declspec(dllexport) void getPreviewFrame(UINT8 * image);

    __declspec(dllexport) tStatusCallbackInfo getStatus();

    __declspec(dllexport) void setROI(const ROIInfo * roiInfo);

    __declspec(dllexport) void setParameter(const char* parameter, INT32 value);
    __declspec(dllexport) INT32 getParameter(const char* parameter);




}
