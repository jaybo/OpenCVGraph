
#include "stdafx.h"
#include "FrameProcessor.h"
#include <iomanip>

using namespace openCVGui;
using namespace cv;

namespace openCVGui
{
	FrameProcessor::FrameProcessor(std::string name, GraphData& data, bool showView)
		: Name(name), firstTime(true), duration(0), tictoc(""), frameToStop(0)
	{
		std::cout << "FrameProcessor()" << std::endl;
		std::string config("config");
		std::wstring widestr(L"config");
		// create the config directory
		_wmkdir((const wchar_t *) widestr.c_str());
		persistFile = config  + "/" + name + ".yml";
		//std::cout << persistFile << std::endl;

	}

	FrameProcessor::~FrameProcessor()
	{
		std::cout << "~FrameProcessor()" << std::endl;
	}

    // Graph is starting up
	// Allocate resources if needed
	void FrameProcessor::init(GraphData& data)
	{
        loadConfig();
        saveConfig();
	}


	bool FrameProcessor::process(GraphData& data)
	{
		firstTime = false;

		// called at start of processing
		// tic();	

		// do all the work here

		// called at the end of processing
		// toc();	
        return true;
	}

    // Graph is stopping
	// Deallocate resources
	void FrameProcessor::fini(GraphData& data)
	{

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
