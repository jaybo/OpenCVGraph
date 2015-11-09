
#include "stdafx.h"

#include "GraphManager.h"

namespace openCVGraph
{
    // Keep a vector of Filters and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    GraphManager::GraphManager(const std::string name, bool abortOnESC)
    {
        m_Name = name;
		gd.m_GraphName = m_Name;
		gd.m_AbortOnESC = abortOnESC;
        m_GraphState = GraphState::Stop;

        std::string config("config");
        fs::create_directory(config);

        // The settings file name combines both the GraphName and the Filter together
        m_persistFile = config + "\\" + m_Name + ".yml";
        std::cout << "GraphManager() file: " << m_persistFile << std::endl;
    }

    GraphManager::~GraphManager()
    {
        std::cout << "~GraphManager()" << std::endl;
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

	bool GraphManager::ProcessOne(int key)
	{
		bool fOK = true;
        
        // first process keyhits
        if (key != -1) {
            for (int i = 0; i < m_Filters.size(); i++) {
                fOK &= m_Filters[i]->processKeyboard(gd, key);
            }
        }

        // MAKE ONE PASS THROUGH THE GRAPH
		for (int i = 0; i < m_Filters.size(); i++) {
			m_Filters[i]->tic();
			// Q: Bail only at end of loop or partway through?
			// Currently, complete the loop
			fOK &= m_Filters[i]->process(gd);
			m_Filters[i]->toc();
            m_Filters[i]->UpdateView();
		}
        gd.m_FrameNumber++;

        // If settings were modified 
		return fOK;
	}

    bool GraphManager::ProcessLoop()
    {
		bool fOK = true;
		m_Stepping = false;

        loadConfig();

		// Init everybody
		for (int i = 0; i < m_Filters.size(); i++) {
			fOK &= m_Filters[i]->init(gd);
			if (!fOK) {
				cout << "ERROR: " + m_Filters[i]->m_CombinedName << " failed init()" << endl;
			}
		}

		// main processing loop
        while (fOK) {
            // This should be the only waitKey() in the entire graph
            int key = cv::waitKey(1);   
            if (gd.m_AbortOnESC && (key == 27)) {
                fOK = false;
                break;
            }

			switch (m_GraphState) {
			case GraphState::Stop:
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(33));
				break;
			case GraphState::Pause:
				if (m_Stepping) {
					fOK &= ProcessOne(key);
					m_Stepping = false;
				}
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(5));
				break;
			case GraphState::Run:
				fOK &= ProcessOne(key);
				break;
			}

        }

        saveConfig();

		// clean up
		for (int i = 0; i < m_Filters.size(); i++) {
			m_Filters[i]->fini(gd);
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

    void GraphManager::saveConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::WRITE);
        if (!fs.isOpened()) { std::cout << "ERROR: unable to open file storage!" << m_persistFile << std::endl; return; }

        for (int i = 0; i < m_Filters.size(); i++) {
            cout << m_Filters[i]->Name;
            fs << m_Filters[i]->Name.c_str() << "{";
            m_Filters[i]->saveConfig(fs, gd);
            fs << "}";
        }
        fs.release();
    }

    void GraphManager::loadConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::READ);
        if (!fs.isOpened()) { std::cout << "ERROR: unable to open file storage!" << m_persistFile << std::endl; return; }

        for (int i = 0; i < m_Filters.size(); i++) {
            auto node = fs[m_Filters[i]->Name.c_str()];
            if (!node.empty()) {
                m_Filters[i]->loadConfig(node, gd);
            }
        }
        fs.release();
    }
}
