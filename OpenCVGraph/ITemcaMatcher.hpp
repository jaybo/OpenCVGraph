#pragma once

typedef struct tMatcherInfo {
    float score;
} MatcherInfo;

//
// All TEMCA Matcher filters must implement this interface
//
class ITemcaMatcher
{ 
public:
    virtual ~ITemcaMatcher() {}
    virtual void setROI(int x, int y, int marginPix) { ; }
    virtual MatcherInfo getMatcherInfo() { return MatcherInfo(); }
};

