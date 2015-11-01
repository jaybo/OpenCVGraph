
#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include "GraphData.h"
#include "FrameProcessor.h"

namespace openCVGui
{
	class FrameLoopProcessor {

	public:
		FrameLoopProcessor(std::string name);
		GraphData *graphData = new GraphData();
		std::vector<FrameProcessor> Processors;
		bool ProcessLoop();

	private:
		std::string Name;
	};
}
