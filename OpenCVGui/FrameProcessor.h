
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
		FrameProcessor(std::string name, GraphData& data, bool showView = false, 
            int width=512, int height=512, int x = 0, int y = 0);
		virtual ~FrameProcessor();

		long frameToStop;

		virtual bool init(GraphData& data);
		virtual bool process(GraphData& data);
		virtual bool fini(GraphData& data);
        virtual bool processKeyboard(GraphData& data);

        virtual void tic();
        virtual void toc();

        virtual void saveConfig();
        virtual void loadConfig();
        
        std::string Name;
		std::string m_CombinedName; // Graph-FrameProcessor name

	protected:
		std::string m_persistFile;
		bool m_firstTime = true;
		bool m_showView = false;
		double duration;
		std::string tictoc;
		cv::Mat imView;
        int m_x, m_y, m_width, m_height;

		ZoomView view; // ("viewA", imView, 1024, 1024, 100, 100);
        cv::MouseCallback m_MouseCallback = NULL;
	};


}
