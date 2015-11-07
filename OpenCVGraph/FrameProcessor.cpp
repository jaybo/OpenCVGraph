
#include "stdafx.h"
#include "FrameProcessor.h"

using namespace std;
namespace fs = ::boost::filesystem;
using namespace cv;

namespace openCVGraph
{
    FrameProcessor::FrameProcessor(std::string name, GraphData& data, bool showView, int width, int height, int x, int y )
        : Name(name), m_showView(showView),
        m_x(x), m_y(y), m_width(width), m_height(height),
        m_firstTime(true), duration(0), tictoc(""), frameToStop(0)
    {
		std::cout << "FrameProcessor()" << std::endl;
		std::string config("config");
		fs::create_directory(config);
		m_CombinedName = data.GraphName + "-" + name;
 
		// The settings file name combines both the GraphName and the FrameProcessor together
        m_persistFile = config  + "/" + m_CombinedName + ".yml";
		std::cout << m_persistFile << std::endl;

		imView = Mat::eye(10, 10, CV_16U);
	}

	FrameProcessor::~FrameProcessor()
	{
		std::cout << "~FrameProcessor()" << std::endl;
	}

    // Graph is starting up
	// Allocate resources if needed
	bool FrameProcessor::init(GraphData& data)
	{
		if (m_showView) {
			view = ZoomView(m_CombinedName, imView );
            view.Init(m_width, m_height, m_x, m_y, m_MouseCallback);
		}
        loadConfig();
        saveConfig();
		return true;
	}

	// All of the work is done here
	bool FrameProcessor::process(GraphData& data)
	{
		m_firstTime = false;

		// do all the work here

        return true;
	}

    // Graph is stopping
	// Deallocate resources
	bool FrameProcessor::fini(GraphData& data)
	{
		return true;
	}

    // keyWait required to make the UI activate
    bool FrameProcessor::processKeyboard(GraphData& data)
    {
        if (m_showView) {
            return view.KeyboardProcessor();
        }
        return true;
    }

	// Record time at start of processing
	void FrameProcessor::tic()
	{
		duration = static_cast<double>(cv::getTickCount());
	}

	// Calc delta at end of processing
	void FrameProcessor::toc()
	{
		duration = (static_cast<double>(cv::getTickCount()) - duration) / cv::getTickFrequency();
		std::cout << Name << "\ttime(sec):" << std::fixed << std::setprecision(6) << duration << std::endl;
	}

	void FrameProcessor::saveConfig()
	{
		FileStorage fs(m_persistFile, FileStorage::WRITE);
		fs << "tictoc" << tictoc.c_str();

		fs.release();
	}

	void FrameProcessor::loadConfig()
	{
		FileStorage fs2(m_persistFile, FileStorage::READ);

		fs2["tictoc"] >> tictoc;

		fs2.release();
	}
}
