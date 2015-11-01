
#include "stdafx.h"
#include "FrameLoopProcessor.h"

namespace openCVGui
{
	FrameLoopProcessor::FrameLoopProcessor(const std::string name)
	{
		Name = name;
	}

	bool FrameLoopProcessor::ProcessLoop()
	{
		return true;
	}
}
