
#pragma once
#pragma warning(disable : 4482)

#include "stdafx.h"

#include "Property.h"
#include "GraphData.h"
#include "Config.h"
#include "ZoomView.h"

namespace openCVGui
{

	class FrameProcessor
	{
	public:
		FrameProcessor(std::string name, GraphData& data, bool showView = false);
		virtual ~FrameProcessor();

		long frameToStop;

		virtual bool init(GraphData& data);
		virtual bool process(GraphData& data);
		virtual bool fini(GraphData& data);

        virtual void tic();
        virtual void toc();

        virtual void saveConfig();
        virtual void loadConfig();
        
        std::string Name;
		std::string CombinedName; // Graph-FrameProcessor name

	protected:
		std::string persistFile;
		bool firstTime = true;
		bool showView = false;
		double duration;
		std::string tictoc;
		cv::Mat imView;

		ZoomView view; // ("viewA", imView, 1024, 1024, 100, 100);
	};


}
