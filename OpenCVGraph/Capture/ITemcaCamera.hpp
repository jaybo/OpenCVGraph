#pragma once

#ifndef INCLUDE_OCVG_ITEMCA_CAMERA_HPP
#define INCLUDE_OCVG_ITEMCA_CAMERA_HPP

//
// All TEMCA cameras must implement this interface
//
class ITemcaCamera
{
public:
    virtual ~ITemcaCamera() {}
    virtual int getWidth() { return 3840; }
    virtual int getHeight() { return 3840; }
    virtual int getFormat() { return CV_16UC1; }
    virtual int getBytesPerPixel() { return 2; }
    virtual const char * getCameraModel() { return "Ximea CB200MG"; }
    virtual const char * getCameraId() { return "SerialNumberGoesHere"; }

    // TEMCA data is always shifted into full range of 16 bits, but
    // the following indicates the actual BPP of the sensor (ie. 12)
    virtual int getActualBpp() { return 12; }

    virtual int getGain() { return 0; }
    virtual void setGain(int value) {}
    virtual int getExposure() { return 0; }
    virtual void setExposure(int value) {}

};

#endif