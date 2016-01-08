
#pragma once

#include "stdafx.h"

using namespace std;
using namespace spdlog;

namespace openCVGraph
{
    // Handles stepping a list of filter graphs in parallel.  
    //
    // All graphs in the list will be stepped in unison (and since each graph runs on its own thread all will run simultaneously)
    // and then WaitStepCompletion will only return when all graphs have completed their work.

    class  GraphParallelStep {
    public:

        GraphParallelStep(string name, list<GraphManager *> graphs) {
            m_Name = name;
            m_Graphs = graphs;
        }

        // Start the thread for each graph and then go to the "Pause" state

        bool init() {
            bool fOK = true;

            for (auto& graph : m_Graphs) {
                if (graph->IsEnabled()) {
                    graph->StartThread();
                    fOK &= graph->GotoState(GraphManager::GraphState::Pause);
                }
            }
            m_Initialized = true;
            return fOK;
        }

        // Abort each graph, and then wait for the thread to complete

        bool fini() {
            bool fOK = true;

            for (auto& graph : m_Graphs) {
                if (graph->IsEnabled()) {
                    graph->Abort();
                    fOK &= graph->GotoState(GraphManager::GraphState::Stop);

                    graph->JoinThread();
                }
            }
            m_Initialized = false;
            return fOK;
        }

        // Step all graphs in parallel.  Triggers an event which wakes up each graph, but call returns immediately. 

        bool Step() {
            bool fOK = true;

            // Step all of the filters
            for (auto& graph : m_Graphs)
            {
                if (graph->IsEnabled()) {
                    fOK &= graph->Step();
                }
            }

            return fOK;
        }

        // Wait for all graphs to finish their work.

        bool WaitStepCompletion()
        {
            bool fOK = true;

            // Wait for them all to complete
            for (auto& graph : m_Graphs)
            {
                if (graph->IsEnabled()) {

                    std::unique_lock<std::mutex> lk(graph->getWaitMutex());
                    while (!graph->CompletedStep() && !graph->IsAborted())
                        graph->getConditionalVariable().wait(lk);
                    fOK &= !graph->IsAborted();
                }
            }

            return fOK;
        }

        string & GetName() {
            return m_Name;
        }

    private:
        string m_Name;
        list<GraphManager *> m_Graphs;
        bool m_Initialized = false;

    };
}
