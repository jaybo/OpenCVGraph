
#pragma once
#pragma warning(disable : 4482)

#include "Property.h"
#include "GraphData.h"
#include "Config.h"
#include "FrameProcessor.h"
#include "OpenCvZoomView.h"

namespace openCVGui
{

	class FrameProcessor
	{
	public:
		FrameProcessor(std::string name, GraphData &gd, bool showView = false);
		~FrameProcessor();

		long frameToStop;

		virtual void process(GraphData& data);

	private:
		std::string Name;
		bool firstTime = true;
		bool showView = false;
		double duration;
		std::string tictoc;

		cv::Mat imView;

		void tic();
		void toc();

		void saveConfig();
		void loadConfig();
	};


}
