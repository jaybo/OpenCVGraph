
#include "stdafx.h"

#include "GraphManager.h"

namespace openCVGraph
{
    // Keep a vector of FrameProcessors and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    GraphManager::GraphManager(const std::string name, bool abortOnESC)
    {
        m_Name = name;
		gd.m_GraphName = m_Name;
		gd.abortOnESC = abortOnESC;
        m_GraphState = GraphState::Stop;
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
        
        // first process keyhits, then the frame
        int c = waitKey(1);
        if (c != -1) {
            for (int i = 0; i < Processors.size(); i++) {
                fOK &= Processors[i]->processKeyboard(gd, c);
            }
        }

        // MAKE ONE PASS THROUGH THE GRAPH
		for (int i = 0; i < Processors.size(); i++) {
			Processors[i]->tic();
			// Q: Bail only at end of loop or partway through?
			// Currently, complete the loop
			fOK &= Processors[i]->process(gd);

			Processors[i]->toc();
            gd.m_FrameNumber++;
		}

        // If settings were modified 
		return fOK;
	}

    bool GraphManager::ProcessLoop()
    {
		bool fOK = true;
		m_Stepping = false;

		// Init everybody
		for (int i = 0; i < Processors.size(); i++) {
			fOK &= Processors[i]->init(gd);
			if (!fOK) {
				cout << "ERROR: " + Processors[i]->m_CombinedName << " failed init()" << endl;
			}
		}

		// main processing loop
        while (fOK) {
			switch (m_GraphState) {
			case GraphState::Stop:
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(33));
				break;
			case GraphState::Pause:
				if (m_Stepping) {
					fOK &= ProcessOne();
					m_Stepping = false;
				}
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(5));
				break;
			case GraphState::Run:
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
        destroyAllWindows();

		return fOK;
	}

    bool GraphManager::Step()
    {
        if (m_GraphState == GraphState::Pause)
        {
            m_Stepping = true;
			return true;
        }
        return false;
    }

    bool GraphManager::GotoState(GraphState newState)
    {
        m_GraphState = newState;

        return true;
    }
}
