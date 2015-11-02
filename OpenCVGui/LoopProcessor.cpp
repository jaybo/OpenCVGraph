
#include "stdafx.h"
#include "LoopProcessor.h"

namespace openCVGui
{
    // Keep a vector of FrameProcessors and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    LoopProcessor::LoopProcessor(const std::string name)
    {
        Name = name;
        state = Stop;
        isInitialized = false;
    }

    bool LoopProcessor::ProcessLoop()
    {
        bool fOK = isInitialized;

        if (fOK) {
            for (int i = 0; i < Processors.size(); i++) {
                Processors[i]->tic();
                // Q: Bail only at end of loop or partway through?
                // Currently, complete the loop
                fOK &= Processors[i]->process(gd);
                Processors[i]->toc();
            }
        }
        return fOK;
    }

    bool LoopProcessor::Step()
    {
        if (state == GraphState::Pause)
        {
            return ProcessLoop();
        }
        return false;
    }

    bool LoopProcessor::GotoState(GraphState newState)
    {
        if (!isInitialized) {
            for (int i = 0; i < Processors.size(); i++) {
                Processors[i]->init(gd);
            }
        }

        state = newState;

        //switch (newState)
        //{
        //case GraphState::Stop:
        //    state = GraphState::Stop;
        //    break;
        //case GraphState::Step:
        //    state = GraphState::Step;
        //    break;
        //case GraphState::Run:
        //    state = GraphState::Run;
        //    break;
        //}

        if (state == Stop && isInitialized) {
            for (int i = 0; i < Processors.size(); i++) {
                Processors[i]->fini(gd);
            }
            isInitialized = false;
        }

        return true;
    }
}
