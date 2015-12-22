#pragma once

#ifndef INCLUDE_OCVG_ITEMCA_CAMERA_HPP
#define INCLUDE_OCVG_ITEMCA_CAMERA_HPP

typedef struct tCameraInfo {
    int width;
    int height;
    int format;             // CV_16UC1
    int pixel_depth;        // bytes per pixel
    int camera_bpp;
    char camera_model[256];
    char camera_id[256];
} CameraInfo;

//
// All TEMCA cameras must implement this interface
//
class ITemcaCamera
{
public:
    virtual ~ITemcaCamera() {}
    virtual CameraInfo getCameraInfo() {
        CameraInfo ci;
        ci.width = 3840;
        ci.height = 3840;
        ci.format =  CV_16UC1;
        ci.pixel_depth = 2;
        ci.camera_bpp = 12;
        strcpy_s (ci.camera_model, "Ximea CB200MG"); 
        strcpy_s (ci.camera_id , "SerialNumberGoesHere"); 
        return ci;
    }

    virtual int getGain() { return 0; }
    virtual void setGain(int value) {}
    virtual int getExposure() { return 0; }
    virtual void setExposure(int value) {}

};

#endif