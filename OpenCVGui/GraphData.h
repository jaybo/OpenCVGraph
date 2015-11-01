
#pragma once

#include <opencv2/opencv.hpp>

namespace openCVGui
{
	class GraphData {
		cv::Mat imCapture;
		cv::Mat imResult;
		std::vector<cv::Mat> imStack;
		int frameCounter = 0;
	};
}
