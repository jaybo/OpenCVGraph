
#pragma once
#include "stdafx.h"

using namespace std;

namespace openCVGui
{
	class  GraphManager {

	public:
        enum GraphState {
            Stop,
            Pause,      // Can only "step" in the Pause state
            Run
        };
        
        GraphManager(std::string name, bool abortOnESC = true);
        ~GraphManager();

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
