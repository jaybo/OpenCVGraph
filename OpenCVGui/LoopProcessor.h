
#pragma once
#include "stdafx.h"
#include "GraphData.h"
#include "FrameProcessor.h"

namespace openCVGui
{
	class  LoopProcessor {

	public:
        enum GraphState {
            Stop,
            Pause,      // Can only "step" in the Pause state
            Run
        };
        
        LoopProcessor(std::string name);
		GraphData gd;
		std::vector<std::shared_ptr<FrameProcessor>> Processors;

        bool GotoState(GraphState newState);
        bool Step();

	private:
        GraphState state;
        bool isInitialized;
		std::string Name;
    
        bool ProcessLoop();
    };
}
