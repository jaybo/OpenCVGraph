
#include "stdafx.h"

#include "GraphManager.h"

namespace openCVGraph
{
    // Keep a vector of FrameProcessors and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    GraphManager::GraphManager(const std::string name, bool abortOnESC)
    {
        Name = name;
		gd.GraphName = Name;
		gd.abortOnESC = abortOnESC;
        state = Stop;
        isInitialized = false;
    }

    GraphManager::~GraphManager()
    {
        cv::destroyAllWindows();
    }


	void GraphManager::StartThread()
	{
		thread = boost::thread(&GraphManager::ProcessLoop, this);
	}

	void GraphManager::JoinThread()
	{
		thread.join();
	}

	bool GraphManager::ProcessOne()
	{
		bool fOK = true;
		for (int i = 0; i < Processors.size(); i++) {
			Processors[i]->tic();
			// Q: Bail only at end of loop or partway through?
			// Currently, complete the loop
			fOK &= Processors[i]->process(gd);
            fOK &= Processors[i]->processKeyboard(gd);
			Processors[i]->toc();
            gd.frameCounter++;
		}
		return fOK;
	}

    bool GraphManager::ProcessLoop()
    {
		bool fOK = true;
		stepping = false;

		// Init everybody
		for (int i = 0; i < Processors.size(); i++) {
			fOK &= Processors[i]->init(gd);
			if (!fOK) {
				cout << "ERROR: " + Processors[i]->m_CombinedName << " failed init()" << endl;
			}
		}

		// main processing loop
        while (fOK) {
			switch (state) {
			case Stop:
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(33));
				break;
			case Pause:
				if (stepping) {
					fOK &= ProcessOne();
					stepping = false;
				}
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(5));
				break;
			case Run:
				fOK &= ProcessOne();
				break;
			}
			auto key = cv::waitKey(1);
			if (gd.abortOnESC && (key == 27)) {
				fOK = false;
			}
        }

		// clean up
		for (int i = 0; i < Processors.size(); i++) {
			Processors[i]->fini(gd);
		}

		return fOK;
	}

    bool GraphManager::Step()
    {
        if (state == GraphState::Pause)
        {
            stepping = true;
			return true;
        }
        return false;
    }

    bool GraphManager::GotoState(GraphState newState)
    {
        state = newState;

        return true;
    }
}
