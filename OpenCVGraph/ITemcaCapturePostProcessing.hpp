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
    virtual void setPreviewDecimationFactor(int decimationFactor) = 0;
    virtual int getPreviewDecimationFactor() = 0;
};

