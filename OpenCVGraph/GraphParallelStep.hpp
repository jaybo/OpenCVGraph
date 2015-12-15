
#pragma once

#include "stdafx.h"

using namespace std;
using namespace spdlog;

namespace openCVGraph
{
    // Handles stepping a list of filter graphs in parallel

    class  GraphParallelStep {
    public:
        GraphParallelStep(string name, list<GraphManager *> graphs) {
            m_Name = name;
            m_Graphs = graphs;
        }

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

        bool fini() {
            bool fOK = true;

            for (auto& graph : m_Graphs) {
                if (graph->IsEnabled()) {
                    graph->Abort();
                    fOK &= graph->GotoState(GraphManager::GraphState::Stop);

                    //graph.StopThread();
                }
            }
            m_Initialized = false;
            return fOK;
        }

        // Step all graphs in parallel.  Return when all graphs have finished.

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

        // The graph is being passed a source image "m_imCapture"
        void NewCaptureImage(GraphData & graphData)
        {
            for (GraphManager * gm : m_Graphs) {
                auto gd = gm->getGraphData();
                gd.m_imCapture = graphData.m_imCapture;
                gd.CopyCaptureToRequiredFormats();
            }
        }

    private:
        string m_Name;
        list<GraphManager *> m_Graphs;
        bool m_Initialized = false;

    };
}
