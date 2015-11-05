
#include "stdafx.h"
#include "FrameProcessor.h"

using namespace std;
namespace fs = ::boost::filesystem;
using namespace cv;

namespace openCVGui
{
	FrameProcessor::FrameProcessor(std::string name, GraphData& data, bool showView)
		: Name(name), showView(showView), firstTime(true), duration(0), tictoc(""), frameToStop(0)
	{
		std::cout << "FrameProcessor()" << std::endl;
		std::string config("config");
		fs::create_directory(config);
		CombinedName = data.GraphName + "-" + name;
 
		// The settings file name combines both the GraphName and the FrameProcessor together
		persistFile = config  + "/" + CombinedName + ".yml";
		std::cout << persistFile << std::endl;

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
		if (showView) {
			view = OpenCvZoomView(CombinedName, imView, 1024, 1024, 100, 100);
		}
        loadConfig();
        saveConfig();
		return true;
	}

	// All of the work is done here
	bool FrameProcessor::process(GraphData& data)
	{
		firstTime = false;

		// do all the work here

        return true;
	}

    // Graph is stopping
	// Deallocate resources
	bool FrameProcessor::fini(GraphData& data)
	{
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
		FileStorage fs(persistFile, FileStorage::WRITE);
		fs << "tictoc" << tictoc.c_str();

		fs.release();
	}

	void FrameProcessor::loadConfig()
	{
		FileStorage fs2(persistFile, FileStorage::READ);

		fs2["tictoc"] >> tictoc;

		fs2.release();
	}
}
