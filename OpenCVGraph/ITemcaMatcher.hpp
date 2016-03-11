#pragma once

typedef struct tMatcherInfo {
    float dX;
    float dY;
    float distance;
    float rotation;
    int good_matches;
} MatcherInfo;

typedef struct tMatcherInfoEdges {
    MatcherInfo EdgeMatches[4] = { 0 };  // Left right top bottom
} MatcherInfoEdges;
//
// All TEMCA Matcher filters must implement this interface
//
class ITemcaMatcher
{ 
public:
    virtual ~ITemcaMatcher() {}
    virtual void grabMatcherTemplate(int x, int y, int width, int height) { ; }
    virtual void grabMatcherEdgeTemplates(int borderPix) { ; }
    virtual MatcherInfo getMatcherInfo() = 0;
    virtual vector<MatcherInfo> getMatcherInfoEdges() = 0;
};

