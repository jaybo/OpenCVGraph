
#pragma once

#ifndef INCLUDE_OCVG_GRAPHMANAGER_HPP
#define INCLUDE_OCVG_GRAPHMANAGER_HPP

#include "stdafx.h"

using namespace std;
namespace spd = spdlog;

namespace openCVGraph
{
    typedef std::shared_ptr < openCVGraph::Filter> Processor;
    class GraphManager;
    typedef bool(*GraphCallback)(GraphManager* graphManager);

    class  GraphManager {
	public:
        enum GraphState {
            Stop,
            Pause,      // Can only "step" in the Pause state
            Run
        };
        
        GraphManager(std::string name, int primaryImageType = CV_8UC3, bool abortOnESC = true, GraphCallback callback = NULL);
        ~GraphManager();

		GraphData m_GraphData;
        GraphData getGraphData() { return m_GraphData; }

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

        void UseCuda(bool useCuda) {
            if (m_GraphState == GraphState::Stop) {
                m_UseCuda = useCuda;
            }
        }

	private:
		std::thread thread;
        GraphState m_GraphState;
		bool m_Stepping;
		std::string m_Name;
        string m_persistFile;
        GraphCallback m_GraphCallback;
        int m_CudaEnabledDeviceCount;
        int m_CudaDeviceIndex = 1;
        bool m_UseCuda = true;
        int m_LogLevel = spd::level::info;
        std::shared_ptr<spdlog::logger> m_Logger;

        bool ProcessLoop();
		ProcessResult ProcessOne(int key);

        void saveConfig();
        void loadConfig();
    };

}
#endif