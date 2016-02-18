#pragma once

// number of elements in the astimatism_profile, MUST ALSO CHANGE TEMCA_GRAPH.PY if this ever changes!!!
#define ASTIGMATISM_SIZE 360

typedef struct tFocusInfo {
    float score;
    float astigmatism;
    float angle;
    float astigmatism_profile[ASTIGMATISM_SIZE];
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

