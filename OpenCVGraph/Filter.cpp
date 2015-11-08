
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
        m_firstTime(true), duration(0), tictoc(""), frameToStop(0)
    {
		std::cout << "Filter()" << std::endl;

		m_CombinedName = data.m_GraphName + "-" + name;
 
		imView = Mat::eye(10, 10, CV_16U);
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
			view = ZoomView(m_CombinedName, imView );
            view.Init(m_width, m_height, m_MouseCallback);
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

    // keyWait required to make the UI activate
    bool Filter::processKeyboard(GraphData& data, int key)
    {
        if (m_showView) {
            return view.KeyboardProcessor(key);
        }
        return true;
    }

	// Record time at start of processing
	void Filter::tic()
	{
		duration = static_cast<double>(cv::getTickCount());
	}

	// Calc delta at end of processing
	void Filter::toc()
	{
		duration = (static_cast<double>(cv::getTickCount()) - duration) / cv::getTickFrequency();
		std::cout << Name << "\ttime(sec):" << std::fixed << std::setprecision(6) << duration << std::endl;
	}

	void Filter::saveConfig(FileStorage fs, GraphData& data)
	{
		fs << "tictoc" << tictoc.c_str();
	}

	void Filter::loadConfig(FileStorage fs, GraphData& data)
	{
		fs["tictoc"] >> tictoc;
	}
}
