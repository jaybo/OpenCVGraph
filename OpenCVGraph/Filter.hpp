#pragma once

#include "stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{

    //
    // Result of calling "process()" on each filter
    //
    enum ProcessResult {
        OK,             // Normal result, continue through the processing loop
        Abort,          // Catastropic error, abort
        Continue,       // GoTo beginning of the loop.  Averaging filters will issue this result until they've accumulated enough images.
    };

    //
    // Filter
    // Performs discrete work on image stream
    //

    class Filter
    {
    public:
        /// Base class for all filters in the graph
        Filter::Filter(std::string name, 
            GraphData& data, 
            StreamIn defaultStreamIn = StreamIn::CaptureRaw, 
            int viewWidth = 512, int viewHeight = 512)
            : m_FilterName(name), m_StreamIn(defaultStreamIn), m_ViewWidth(viewWidth), m_ViewHeight(viewHeight)
        {
            m_Logger = data.m_Logger;

            m_CombinedName = data.m_GraphName + "-" + name;
            m_Logger->info("Filter() " + m_CombinedName);
            m_TickFrequency = cv::getTickFrequency();
            m_imView = Mat::eye(4, 4, CV_16U);
        }

        virtual Filter::~Filter()
        {
            m_Logger->info("~Filter() " + m_CombinedName);
        }

        // Graph is starting up
        // Allocate resources if needed
        // Specify capture and result format(s) required
        // Register mouse callback
        virtual bool Filter::init(GraphData& data)
        {
            if (m_Enabled) {
                if (m_showView) {
                    m_ZoomView = ZoomView(m_CombinedName);
                    m_ZoomView.Init(m_ViewWidth, m_ViewHeight, m_MouseCallback);
                }

                m_IsInitialized = true;
            }
            return true;
        }

        // All of the work is done here
        virtual ProcessResult Filter::process(GraphData& data)
        {
            m_firstTime = false;

            // do all the work here

            return ProcessResult::OK;
        }

        virtual void processView(GraphData& graphData) {
            if (m_showView) {
                m_ZoomView.processView(m_imView, m_imViewTextOverlay, graphData, m_ZoomViewLockIndex);
            }
        };

        // Graph is stopping
        // Deallocate resources
        virtual bool Filter::fini(GraphData& data)
        {
            m_IsInitialized = false;
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
            m_DurationMSSum += m_DurationMS;
            m_DurationMSMax = max(m_DurationMS, m_DurationMSMax);
            m_DurationMSMin = min(m_DurationMS, m_DurationMSMin);
        }

        virtual void Filter::saveConfig(FileStorage& fs, GraphData& data)
        {
            fs << "IsEnabled" << m_Enabled;
            fs << "ShowView" << m_showView;
            fs << "ShowViewControls" << m_showViewControls;
            fs << "ZoomViewLockIndex" << m_ZoomViewLockIndex;

            // cvWriteComment((CvFileStorage *)*fs, "InputStream: 0=RawCapture, 1=Corrected, 2=Output prev filter", 0);
            fs << "InputStream" << (int) m_StreamIn;

            // Save how long the filter took to process its last sample, mean, min, max
            // Help, take me back to C#!!! or even javascript
            std::stringstream strT;
            std::string tmp;
            strT << fixed << setprecision(1);

            strT << (m_DurationMSSum / ((data.m_FrameNumber == 0) ? 1 : data.m_FrameNumber));
            tmp = strT.str();
            fs << "Duration_MS_Mean" << tmp.c_str();
            m_Logger->info("Duration_MS_Mean: " + tmp);

            strT.str("");
            strT << m_DurationMSMin;
            tmp = strT.str();
            fs << "Duration_MS_Min" << tmp.c_str();
            m_Logger->info("Duration_MS_Min: " + tmp);

            strT.str("");
            strT << m_DurationMSMax;
            tmp = strT.str();
            fs << "Duration_MS_Max" << tmp.c_str();
            m_Logger->info("Duration_MS_Max: " + tmp);

            strT.str("");
            strT << m_DurationMS;
            tmp = strT.str();
            fs << "Duration_MS_Last" << tmp.c_str();
            m_Logger->info("Duration_MS_Last: " + tmp);

        }

        virtual void Filter::loadConfig(FileNode& fs, GraphData& data)
        {
            auto en = fs["IsEnabled"];
            if (!en.empty()) {
                en >> m_Enabled;
            }
            fs["ShowView"] >> m_showView;
            fs["ShowViewControls"] >> m_showViewControls;
            fs["ZoomViewLockIndex"] >> m_ZoomViewLockIndex;
            int temp;   // can't >> enums!
            fs["InputStream"] >> temp;
            m_StreamIn = (StreamIn) temp;

            if (m_ZoomViewLockIndex >= MAX_ZOOMVIEW_LOCKS) {
                m_ZoomViewLockIndex = -1;
            }
        }

        virtual void Filter::Enable(bool enable)
        {
            m_Enabled = enable;
        }

        /// Can only be called before the graph is first started
        virtual void Filter::EnableZoomView(bool enable)
        {
            if (!m_IsInitialized) {
                m_showView = enable;
            }
        }

        void Filter::DrawOverlayText(string str, cv::Point p, double scale, CvScalar color = CV_RGB(255,255,255))
        {
            if (m_imViewTextOverlay.empty())
            {
                m_imViewTextOverlay = Mat(m_ViewHeight, m_ViewWidth, CV_8UC3);
            }

            cv::putText(m_imViewTextOverlay, str, p, CV_FONT_HERSHEY_DUPLEX, scale, color);
        }

        void Filter::ClearOverlayText()
        { 
            if (m_imViewTextOverlay.empty())
            {
                m_imViewTextOverlay = Mat(m_ViewHeight, m_ViewWidth, CV_8UC3);
            }
            m_imViewTextOverlay.setTo(0);
        }

        // At the start of each loop, the input stream can be changed.
        void Filter::SetInputStream(StreamIn inputStream) {
            m_StreamIn = inputStream;
        }

        StreamIn Filter::GetInputStream() {
            return m_StreamIn;
        }

        bool Filter::IsEnabled() { return m_Enabled; }

        std::string m_FilterName;
		std::string m_CombinedName; // Graph-Filter name

        double m_DurationMS = 0;                // tictoc of last process
        double m_DurationMSSum = 0;             // sum of all durations
        double m_DurationMSMin = 9999;             
        double m_DurationMSMax = 0;

        int m_ZoomViewLockIndex = 0;            // Lock window zooms together

	protected:
        StreamIn m_StreamIn;                    // what stream should the filter process? Capture, Processed, or Out
		bool m_firstTime = true;
		bool m_showView = false;
        bool m_showViewControls = false;        // view sliders
        bool m_Enabled = true;
        bool m_IsInitialized = false;
        double m_TimeStart;
        double m_TimeEnd;

        double m_TickFrequency;

        int m_ViewWidth, m_ViewHeight;

        cv::Mat m_imView;               // image to display
        cv::Mat m_imViewTextOverlay;        // overlay for that image
        ZoomView m_ZoomView;
        cv::MouseCallback m_MouseCallback = NULL;

        std::shared_ptr<spdlog::logger> m_Logger;
	};

    typedef std::shared_ptr<Filter> CvFilter;

}
