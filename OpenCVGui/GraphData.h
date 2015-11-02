
#pragma once

#include "stdafx.h"

namespace openCVGui
{
	class  GraphData {
	public:
		
		cv::Mat imCapture;
		cv::Mat imResult;
		std::vector<cv::Mat> imStack;
		int frameCounter = 0;

	};
}
