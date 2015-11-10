
#pragma once
#include "stdafx.h"

using namespace std;

namespace openCVGraph
{
    typedef std::shared_ptr < Filter> Processor;
    typedef bool(*GraphCallback)(GraphData data);

    class  GraphManager {

	public:
        enum GraphState {
            Stop,
            Pause,      // Can only "step" in the Pause state
            Run
        };
        
        GraphManager(std::string name, bool abortOnESC = true, GraphCallback callback = NULL);
        ~GraphManager();

		GraphData gd;
		std::vector<Processor> m_Filters;

		void StartThread();
		void JoinThread();

        bool GotoState(GraphState newState);
        bool Step();

        bool AddFilter(Processor filter) {
            if (m_GraphState == GraphState::Stop) {
                m_Filters.push_back(filter);
                return true;
            }
            else return false;
        }

        bool RemoveFilter(Processor filter) {
            if (m_GraphState == GraphState::Stop) {
                m_Filters.erase(std::remove(m_Filters.begin(), m_Filters.end(), filter), m_Filters.end());
                m_Filters.push_back(filter);
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
        GraphCallback m_GraphCallback;
        int m_CudaEnabledDeviceCount;
        int m_CudaDeviceIndex;

        bool ProcessLoop();
		bool ProcessOne(int key);

        void saveConfig();
        void loadConfig();
    };
}
