
#pragma once
// #pragma warning(disable : 4482)

#include "stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{

	class Filter
	{
	public:
		Filter(std::string name, GraphData& data, bool showView = false, 
            int width=512, int height=512);
		virtual ~Filter();

		long frameToStop;

		virtual bool init(GraphData& data);
		virtual bool process(GraphData& data);
		virtual bool fini(GraphData& data);
        virtual bool processKeyboard(GraphData& data, int key);

        void tic();
        void toc();

        virtual void saveConfig(FileStorage &fs, GraphData& data);
        virtual void loadConfig(FileNode &fs, GraphData& data);
        
        std::string Name;
		std::string m_CombinedName; // Graph-Filter name

        void UpdateView(GraphData graphData) {
            if (m_showView) {
                view.UpdateView(m_imView, graphData);
            }
        };
        double m_DurationMS;

	protected:
		bool m_firstTime = true;
		bool m_showView = false;
        double m_TimeStart;
        double m_TimeEnd;

        double m_TickFrequency;
		cv::Mat m_imView;
        int m_width, m_height;

		ZoomView view; 
        cv::MouseCallback m_MouseCallback = NULL;
	};

    typedef std::shared_ptr<Filter> CvFilter;

}
