#pragma once

typedef struct tQCInfo {
    int min_value;
    int max_value;
    int mean_value;
    int histogram[256];
} QCInfo;

//
// All TEMCA Focus filters must implement this interface
//
class ITemcaQC
{
public:
    virtual ~ITemcaQC() {}
    virtual QCInfo getQCInfo() { return QCInfo(); }
};

