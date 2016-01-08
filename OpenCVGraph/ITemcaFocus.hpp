#pragma once

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

