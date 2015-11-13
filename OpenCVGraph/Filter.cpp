
#include "stdafx.h"
#include "Filter.h"

using namespace std;
namespace fs = ::boost::filesystem;
using namespace cv;

namespace openCVGraph
{
    Filter::Filter(std::string name, GraphData& data, bool showView, int width, int height)
        : Name(name), m_showView(showView),
        m_width(width), m_height(height),
        m_firstTime(true), m_DurationMS(0), frameToStop(0)
    {
		std::cout << "Filter()" << std::endl;

		m_CombinedName = data.m_GraphName + "-" + name;
        m_TickFrequency = cv::getTickFrequency();
		m_imView = Mat::eye(10, 10, CV_16U);
	}

	Filter::~Filter()
	{
		std::cout << "~Filter()" << std::endl;
	}

    // Graph is starting up
	// Allocate resources if needed
	bool Filter::init(GraphData& data)
	{
		if (m_showView) {
            m_ZoomView = ZoomView(m_CombinedName);
            m_ZoomView.Init(m_width, m_height, m_MouseCallback);
		}

		return true;
	}

	// All of the work is done here
	bool Filter::process(GraphData& data)
	{
		m_firstTime = false;

		// do all the work here

        return true;
	}

    // Graph is stopping
	// Deallocate resources
	bool Filter::fini(GraphData& data)
	{
		return true;
	}

    // Process keyboard hits
    bool Filter::processKeyboard(GraphData& data, int key)
    {
        if (m_showView) {
            return m_ZoomView.KeyboardProcessor(key);
        }
        return true;
    }

	// Record time at start of processing
	void Filter::tic()
	{
		m_TimeStart = static_cast<double>(cv::getTickCount());
	}

	// Calc delta at end of processing
	void Filter::toc()
	{
        m_TimeEnd = static_cast<double>(cv::getTickCount());
		m_DurationMS = (m_TimeEnd - m_TimeStart) / m_TickFrequency * 1000;
		std::cout << Name << "\ttime(MS): " << std::fixed << std::setprecision(1) << m_DurationMS << std::endl;
	}

	void Filter::saveConfig(FileStorage& fs, GraphData& data)
	{
	}

	void Filter::loadConfig(FileNode& fs, GraphData& data)
	{
	}
}
