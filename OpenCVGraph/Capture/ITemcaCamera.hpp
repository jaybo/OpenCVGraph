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
    virtual int getWidth() = 0;
    virtual int getHeight() = 0;

    // TEMCA data is always shifted into full range of 16 bits, but
    // the following indicates the actual BPP of the sensor (ie. 12)
    virtual int getActualBpp() = 0;

    virtual int getGain() = 0;
    virtual void setGain(int value) = 0;
    virtual int getExposure() = 0;
    virtual void setExposure(int value) = 0;

};

#endif