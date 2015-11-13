
#pragma once

#ifndef INCLUDE_FILTER_HPP
#define INCLUDE_FILTER_HPP

#include "stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{

	class Filter
	{
	public:
        /// Base class for all filters in the graph
        Filter::Filter(std::string name, GraphData& data, bool showView = false, int width = 512, int height=512)
            : m_FilterName(name), m_showView(showView),
            m_width(width), m_height(height),
            m_firstTime(true), m_DurationMS(0), frameToStop(0)
        {
            BOOST_LOG_TRIVIAL(info) << "Filter() " << m_FilterName;

            m_CombinedName = data.m_GraphName + "-" + name;
            m_TickFrequency = cv::getTickFrequency();
            m_imView = Mat::eye(10, 10, CV_16U);
        }

        virtual Filter::~Filter()
        {
            BOOST_LOG_TRIVIAL(info) << "~Filter() " << m_FilterName;
        }

        // Graph is starting up
        // Allocate resources if needed
        virtual bool Filter::init(GraphData& data)
        {
            if (m_showView) {
                m_ZoomView = ZoomView(m_CombinedName);
                m_ZoomView.Init(m_width, m_height, m_MouseCallback);
            }

            return true;
        }

        // All of the work is done here
        virtual bool Filter::process(GraphData& data)
        {
            m_firstTime = false;

            // do all the work here

            return true;
        }

        virtual void UpdateView(GraphData graphData) {
            if (m_showView) {
                m_ZoomView.UpdateView(m_imView, m_imViewOverlay, graphData);
            }
        };

        // Graph is stopping
        // Deallocate resources
        virtual bool Filter::fini(GraphData& data)
        {
            return true;
        }

        // Process keyboard hits
        virtual bool Filter::processKeyboard(GraphData& data, int key)
        {
            if (m_showView) {
                return m_ZoomView.KeyboardProcessor(key);
            }
            return true;
        }

        // Record time at start of processing
        virtual void Filter::tic()
        {
            m_TimeStart = static_cast<double>(cv::getTickCount());
        }

        // Calc delta at end of processing
        virtual void Filter::toc()
        {
            m_TimeEnd = static_cast<double>(cv::getTickCount());
            m_DurationMS = (m_TimeEnd - m_TimeStart) / m_TickFrequency * 1000;

            BOOST_LOG_TRIVIAL(info) << m_FilterName << "\ttime(MS): " << std::fixed << std::setprecision(1) << m_DurationMS;
        }

        virtual void Filter::saveConfig(FileStorage& fs, GraphData& data)
        {
            // Save how long the filter took to process its last sample
            // Help, take me back to C#!!! or even javascript
            std::stringstream strDuration;
            strDuration << fixed << setprecision(1) << m_DurationMS;
            const std::string tmp = strDuration.str();
            const char* cstr = tmp.c_str();
            fs << "LastDurationMS" << cstr;
            fs << "IsEnabled" << m_Enabled;
        }

        virtual void Filter::loadConfig(FileNode& fs, GraphData& data)
        {
            auto en = fs["IsEnabled"];
            if (!en.empty()) {
                en >> m_Enabled;
            }
        }

        virtual void Filter::Enable(bool enable)
        {
            m_Enabled = enable;
        }

        bool Filter::IsEnabled() { return m_Enabled; }

		long frameToStop;
        
        std::string m_FilterName;
		std::string m_CombinedName; // Graph-Filter name

        double m_DurationMS;

	protected:
		bool m_firstTime = true;
		bool m_showView = false;
        bool m_Enabled = true;
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
#endif