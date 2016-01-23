#pragma once

//
// All TEMCA CapturePostProcessing filters must implement this interface
//
class ITemcaCapturePostProcessing
{
public:
    virtual ~ITemcaCapturePostProcessing() {}
    virtual void setBrightDarkCorrectionEnabled(bool enable) = 0;
    virtual bool getBrightDarkCorrectionEnabled() = 0;
};

