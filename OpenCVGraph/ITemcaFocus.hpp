#pragma once

#ifndef INCLUDE_OCVG_ITEMCA_FOCUS_HPP
#define INCLUDE_OCVG_ITEMCA_FOCUS_HPP

typedef struct tFocusInfo {
    float score;
    float astigmatism;
    float angle;
} FocusInfo;

//
// All TEMCA Focus filters must implement this interface
//
class ITemcaFocus
{
public:
    virtual ~ITemcaFocus() {}
    virtual FocusInfo getFocusInfo() { return FocusInfo(); }
};

#endif