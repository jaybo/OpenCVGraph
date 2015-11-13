
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
                m_ZoomView.UpdateView(m_imView, m_imViewOverlay, graphData);
            }
        };
        double m_DurationMS;

	protected:
		bool m_firstTime = true;
		bool m_showView = false;
        double m_TimeStart;
        double m_TimeEnd;

        double m_TickFrequency;

        int m_width, m_height;

        cv::Mat m_imView;               // image to display
        cv::Mat m_imViewOverlay;        // overlay for that image
        ZoomView m_ZoomView;
        cv::MouseCallback m_MouseCallback = NULL;
	};

    typedef std::shared_ptr<Filter> CvFilter;

}
