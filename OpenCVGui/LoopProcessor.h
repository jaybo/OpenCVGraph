
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
        
        LoopProcessor(std::string name, bool abortOnESC = true);
		GraphData gd;
		std::vector<std::shared_ptr<FrameProcessor>> Processors;

		void StartThread();
		void JoinThread();

        bool GotoState(GraphState newState);
        bool Step();

	private:
		boost::thread thread;
        GraphState state;
        bool isInitialized;
		bool stepping;
		std::string Name;
    
        bool ProcessLoop();
		bool ProcessOne();
    };
}
