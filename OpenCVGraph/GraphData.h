
#pragma once

#include "stdafx.h"

using namespace std;

namespace openCVGui
{
	class  GraphData {
	public:
		std::string GraphName;			// Name of the loop processor running this graph
		bool abortOnESC;
		cv::Mat imCapture;
		cv::Mat imResult;
		std::vector<cv::Mat> imStack;
		int frameCounter = 0;

	};
}
