
#pragma once
#include "stdafx.h"

using namespace std;

namespace openCVGraph
{
    typedef std::shared_ptr < FrameProcessor> Processor;
    
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
		std::vector<Processor> Processors;

		void StartThread();
		void JoinThread();

        bool GotoState(GraphState newState);
        bool Step();

        bool AddFilter(Processor filter) {
            if (state == GraphState::Stop) {
                Processors.push_back(filter);
                return true;
            }
            else return false;
        }

        bool RemoveFilter(Processor filter) {
            if (state == GraphState::Stop) {
                Processors.erase(std::remove(Processors.begin(), Processors.end(), filter), Processors.end());
                Processors.push_back(filter);
                return true;
            }
            else return false;
        }

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
