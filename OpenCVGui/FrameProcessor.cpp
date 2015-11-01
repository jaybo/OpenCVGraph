
#include "FrameProcessor.h"
#include <iomanip>

using namespace openCVGui;
using namespace cv;

namespace openCVGui
{
	FrameProcessor::FrameProcessor(std::string name, GraphData &data, bool showView)
		: firstTime(true), duration(0), tictoc(""), frameToStop(0)
	{
		std::cout << "FrameProcessor()" << std::endl;

		loadConfig();
		saveConfig();
	}

	FrameProcessor::~FrameProcessor()
	{
		std::cout << "~FrameProcessor()" << std::endl;
	}


	void FrameProcessor::process(GraphData& data)
	{
		firstTime = false;

		tic();


		//process();

		toc();
	}



	void FrameProcessor::tic()
	{
		duration = static_cast<double>(cv::getTickCount());
	}

	void FrameProcessor::toc()
	{
		duration = (static_cast<double>(cv::getTickCount()) - duration) / cv::getTickFrequency();
		std::cout << Name << "\ttime(sec):" << std::fixed << std::setprecision(6) << duration << std::endl;
	}

	void FrameProcessor::saveConfig()
	{
		FileStorage fs(Name + ".yml", FileStorage::WRITE);
		fs << "tictoc" << tictoc.c_str();

		fs.release();
	}

	void FrameProcessor::loadConfig()
	{
		FileStorage fs2(Name + ".yml", FileStorage::READ);

		fs2["tictoc"] >> tictoc;

		fs2.release();
	}
}
