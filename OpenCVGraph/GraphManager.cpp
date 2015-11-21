
#include "stdafx.h"


namespace openCVGraph
{
    // Keep a vector of Filters and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    GraphManager::GraphManager(const std::string name, int primaryImageType, bool abortOnESC, GraphCallback callback)
    {
        m_Name = name;
        gd.m_GraphName = m_Name;
        gd.m_AbortOnESC = abortOnESC;
        m_GraphCallback = callback;

        // Other types may be requested by individual filters
        switch (primaryImageType) {
        case CV_8UC1: gd.m_NeedCV_8UC1 = true; break;
        case CV_8UC3: gd.m_NeedCV_8UC3 = true; break;
        case CV_16UC1: gd.m_NeedCV_16UC1 = true; break;
        case CV_32FC1: gd.m_NeedCV_32FC1 = true; break;
        }

        m_GraphState = GraphState::Stop;
        m_CudaEnabledDeviceCount = cv::cuda::getCudaEnabledDeviceCount();
        if (m_CudaEnabledDeviceCount > 0 && m_CudaDeviceIndex <= m_CudaEnabledDeviceCount) {
           // bugbug todo cv::cuda::setDevice(m_CudaEnabledDeviceCount);
        }

        std::string config("config");
        createDir();

        // The settings file name combines both the GraphName and the Filter together
        m_persistFile = config + "/" + m_Name + ".yml";

        //logging::core::get()->set_filter
        //    (
        //        logging::trivial::severity >= logging::trivial::info
        //        );


        std::cout << "GraphManager() file: " << m_persistFile << std::endl;
    }

    GraphManager::~GraphManager()
    {
        std::cout << "~GraphManager()" << std::endl;
        cv::destroyAllWindows();
    }


    void GraphManager::StartThread()
    {
        thread = std::thread::thread(&GraphManager::ProcessLoop, this);
    }

    void GraphManager::JoinThread()
    {
        thread.join();
    }

    ProcessResult GraphManager::ProcessOne(int key)
    {
        bool fOK = true;
        ProcessResult result = ProcessResult::OK;

        // first process keyhits
        if (key != -1) {
            for (int i = 0; i < m_Filters.size(); i++) {
                fOK &= m_Filters[i]->processKeyboard(gd, key);
            }
        }

        // MAKE ONE PASS THROUGH THE GRAPH
        for (int i = 0; i < m_Filters.size(); i++) {
            Processor filter = m_Filters[i];
            filter->tic();
            // Q: Bail only at end of loop or partway through?
            // Currently, complete the loop
            if (filter->IsEnabled())
            {
                result = filter->process(gd);
            }
            filter->toc();
            if (filter->IsEnabled())
            {
                filter->processView(gd);
            }
            if (result != ProcessResult::OK) break;
        }
        gd.m_FrameNumber++;

        return result;
    }

    bool GraphManager::ProcessLoop()
    {
        bool fOK = true;
        m_Stepping = false;
        ProcessResult result;

        loadConfig();
        
        // Let the filters know whether or not to use Cuda
        gd.m_UseCuda = m_UseCuda;

        // Init everybody
        for (int i = 0; i < m_Filters.size(); i++) {
            fOK &= m_Filters[i]->init(gd);
            if (!fOK) {
                //BOOST_LOG_TRIVIAL(error) << "ERROR: " + m_Filters[i]->m_CombinedName << " failed init()";
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
                std::this_thread::sleep_for(std::chrono::milliseconds(33));
                break;
            case GraphState::Pause:
                if (m_Stepping) {
                    result = ProcessOne(key);
                    fOK = fOK && (result != ProcessResult::Abort);
                    m_Stepping = false;
                }
                // Snooze.  But this should be a mutex or awaitable object
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                break;
            case GraphState::Run:
                result = ProcessOne(key);
                fOK = fOK && (result != ProcessResult::Abort);
                break;
            }

            // Callback to client app?
            if (m_GraphCallback) {
                fOK = (*m_GraphCallback)(this);
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

        // Save state for the graph manager
        fs << "GraphManager" << "{";
        // Persist the filter data
        fs << "CudaEnabledDeviceCount" << m_CudaEnabledDeviceCount;
        fs << "CudaDeviceIndex" << m_CudaDeviceIndex;
        fs << "UseCuda" << m_UseCuda;
        fs << "}";

        // Save state for each filter
        for (int i = 0; i < m_Filters.size(); i++) {
            Processor filter = m_Filters[i];
            cout << filter->m_FilterName;
            fs << filter->m_FilterName.c_str() << "{";
            // Persist the filter data
            filter->saveConfig(fs, gd);
            fs << "}";
        }
        fs.release();
    }

    void GraphManager::loadConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::READ);
        if (!fs.isOpened()) { std::cout << "ERROR: unable to open file storage!" << m_persistFile << std::endl; return; }

        auto node = fs["GraphManager"];
        node["CudaDeviceIndex"] >> m_CudaDeviceIndex;

        if (!node["UseCuda"].empty()) {
            node["UseCuda"] >> m_UseCuda;
        }

        for (int i = 0; i < m_Filters.size(); i++) {
            Processor filter = m_Filters[i];
            auto node = fs[filter->m_FilterName.c_str()];
            if (!node.empty()) {
                filter->loadConfig(node, gd);
            }
        }
        fs.release();
    }
}
