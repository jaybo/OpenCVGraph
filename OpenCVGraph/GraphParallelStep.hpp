#pragma once

#include "stdafx.h"
#include <future> 

using namespace std;
using namespace spdlog;

namespace openCVGraph
{
    typedef std::function<bool(int, int, char *)> tPythonCallbackFunction;

    // Handles stepping a list of filter graphs in parallel.  
    //
    // All graphs in the list will be stepped in unison (and since each graph runs on its own thread all will run simultaneously)
    // and then WaitStepCompletion() will only return when all graphs have completed their work.

    class  GraphParallelStep {
    public:

        GraphParallelStep(string name,
            list<GraphManager *> graphs,
            int completionEventId = -1,
            bool runAsync = false,
            tPythonCallbackFunction pythonCallbackFunc = NULL)
        {
            m_Name = name;
            m_Graphs = graphs;
            m_CompletionEventId = completionEventId;
            m_RunAsync = runAsync;
            m_PythonCallbackFunc = pythonCallbackFunc;
        }

        // Start the thread for each graph and then go to the "Pause" state

        bool init() {
            bool fOK = true;

            for (auto& graph : m_Graphs) {
                graph->StartThread();
                // Ugly!!!  Wait for the graph to fully initialize
                while (!graph->IsInitialized()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                fOK &= graph->GotoState(GraphManager::GraphState::Pause);
            }
            m_Thread = std::thread::thread(&GraphParallelStep::WaitCompletionThenCallPython, this);

            m_Initialized = true;
            return fOK;
        }

        // Abort each graph, and then wait for the thread to complete

        bool fini() {
            bool fOK = true;

            for (auto& graph : m_Graphs) {
                graph->Abort();
                fOK &= graph->GotoState(GraphManager::GraphState::Stop);

                graph->JoinThread();
            }
            m_Initialized = false;
            return fOK;
        }

        // Step all graphs in parallel.  Triggers an event which wakes up each graph, but call returns immediately. 

        bool Step() {
            bool fOK = true;
            m_Stepping = true;

            // Step all of the filters
            for (auto& graph : m_Graphs)
            {
                if (graph->IsEnabled()) {
                    m_HasStepped = true;
                    fOK &= graph->Step();
                }
            }

            // wake up the loop to wait for all graphs to complete and then callback to python
            m_cv.notify_all();

            return fOK;
        }

        // Just exists to signal completion of the step and perform the callback to Python
        bool WaitCompletionThenCallPython()
        {   
            bool fOK = true;

            while (fOK) {
                std::unique_lock<std::mutex> lk(m_mtx);
                m_cv.wait(lk, [=]() {return (m_Stepping == true); });  // return false to continue waiting! 

                // wait for all graphs to complete
                for (auto& graph : m_Graphs)
                {
                    if (graph->IsEnabled()) {

                        std::unique_lock<std::mutex> lk(graph->getWaitMutex());
                        while (!graph->CompletedStep() && !graph->IsAborted())
                            graph->getConditionalVariable().wait(lk);
                        fOK &= !graph->IsAborted();
                        // cout << "WaitCompletionCallPython: done: " + graph->GetName() << std::endl;
                    }
                }
                m_Stepping = false;

                // everybody is done (or failed) callback to python
                if (m_PythonCallbackFunc != NULL) {
                    // -1 means somebody else higher up the foodchain wants to signal the completion event
                    if (m_CompletionEventId != -1) {
                        (m_PythonCallbackFunc)(m_CompletionEventId, 0, "");
                        // cout << "WaitCompletionCallPython: callback done: " + this->GetName() << std::endl;
                    }
                }
            }
            cout << "WaitCompletionCallPython: exited" << std::endl;

            return fOK;
        }


        // Wait for all graphs to finish their work.
        // Either wait on async steps or sync steps

        bool WaitStepCompletion(bool async = false)
        {
            bool fOK = true;
            if (m_HasStepped && (m_RunAsync == async)) {
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
            }

            return fOK;
        }


        string & GetName() { return m_Name; }
        int GetCompletionEventId() { return m_CompletionEventId; }
        list<GraphManager *> m_Graphs;
        bool RunningAsync() { return m_RunAsync; }

    private:
        string m_Name;
        bool m_Initialized = false;
        bool m_RunAsync;                // if True, allow overlap with capture step
        int m_CompletionEventId;        // Id of Event fired when graph step completes in Python land
        bool m_Stepping = false;
        bool m_HasStepped = false;

        tPythonCallbackFunction m_PythonCallbackFunc;
        std::thread m_Thread;
        std::mutex m_mtx;
        std::condition_variable m_cv;           
    };
}
