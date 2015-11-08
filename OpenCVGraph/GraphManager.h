
#pragma once
#include "stdafx.h"

using namespace std;

namespace openCVGraph
{
    typedef std::shared_ptr < Filter> Processor;
    
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
            if (m_GraphState == GraphState::Stop) {
                Processors.push_back(filter);
                return true;
            }
            else return false;
        }

        bool RemoveFilter(Processor filter) {
            if (m_GraphState == GraphState::Stop) {
                Processors.erase(std::remove(Processors.begin(), Processors.end(), filter), Processors.end());
                Processors.push_back(filter);
                return true;
            }
            else return false;
        }

	private:
		boost::thread thread;
        GraphState m_GraphState;
		bool m_Stepping;
		std::string m_Name;
        string m_persistFile;

        bool ProcessLoop();
		bool ProcessOne();

        void saveConfig();
        void loadConfig();
    };
}
