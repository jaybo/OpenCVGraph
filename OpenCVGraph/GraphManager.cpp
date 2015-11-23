
#include "stdafx.h"

namespace spd = spdlog;

namespace openCVGraph
{
    // Keep a vector of Filters and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    GraphManager::GraphManager(const std::string name, int primaryImageType, bool abortOnESC, GraphCallback callback)
    {
        // Set up logging
        try
        {
            string logDir = "logs";
            createDir(logDir);
            std::vector<spdlog::sink_ptr> sinks;
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
            sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(logDir + "/" + "logfile", "txt", 23, 59));
            m_Logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
            spdlog::register_logger(m_Logger);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Log failed: " << ex.what() << std::endl;
        }

        m_Name = name;
        m_GraphData.m_GraphName = m_Name;
        m_GraphData.m_AbortOnESC = abortOnESC;
        m_GraphCallback = callback;
        m_GraphData.m_Logger = m_Logger;

        // Other types may be requested by individual filters
        switch (primaryImageType) {
        case CV_8UC1: m_GraphData.m_NeedCV_8UC1 = true; break;
        case CV_8UC3: m_GraphData.m_NeedCV_8UC3 = true; break;
        case CV_16UC1: m_GraphData.m_NeedCV_16UC1 = true; break;
        case CV_32FC1: m_GraphData.m_NeedCV_32FC1 = true; break;
        }

        m_GraphData.m_PrimaryImageType = primaryImageType;

        m_GraphState = GraphState::Stop;
        m_CudaEnabledDeviceCount = cv::cuda::getCudaEnabledDeviceCount();
        if (m_CudaEnabledDeviceCount > 0 && m_CudaDeviceIndex <= m_CudaEnabledDeviceCount) {
           // bugbug todo cv::cuda::setDevice(m_CudaEnabledDeviceCount);
        }

        std::string config("config");
        createDir(config);

        // The settings file name combines both the GraphName and the Filter together
        m_persistFile = config + "/" + m_Name + ".yml";

        m_GraphData.m_Logger->info() << "GraphManager() file: " << m_persistFile;
    }

    GraphManager::~GraphManager()
    {
        m_GraphData.m_Logger->info() << "~GraphManager()";
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
                fOK &= m_Filters[i]->processKeyboard(m_GraphData, key);
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
                result = filter->process(m_GraphData);
            }
            filter->toc();
            if (filter->IsEnabled())
            {
                filter->processView(m_GraphData);
            }
            if (result != ProcessResult::OK) break;
        }
        m_GraphData.m_FrameNumber++;

        return result;
    }

    bool GraphManager::ProcessLoop()
    {
        bool fOK = true;
        m_Stepping = false;
        ProcessResult result;

        loadConfig();
        
        // Let the filters know whether or not to use Cuda
        m_GraphData.m_UseCuda = m_UseCuda;

        // Init everybody
        for (int i = 0; i < m_Filters.size(); i++) {
            fOK &= m_Filters[i]->init(m_GraphData);
            if (!fOK) {
                m_GraphData.m_Logger->error() << "ERROR: " + m_Filters[i]->m_CombinedName << " failed init()";
            }
        }

        // main processing loop
        while (fOK) {
            // This should be the only waitKey() in the entire graph
            int key = cv::waitKey(1);
            if (m_GraphData.m_AbortOnESC && (key == 27)) {
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
            m_Filters[i]->fini(m_GraphData);
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
        if (!fs.isOpened()) { m_GraphData.m_Logger->error() << "ERROR: unable to open file storage!" << m_persistFile; return; }

        // Save state for the graph manager
        fs << "GraphManager" << "{";
        // Persist the filter data
        cvWriteComment((CvFileStorage *)*fs, "Log Levels: 0=trace, 1=debug, 2=info, 3=notice, 4=warn, 5=err, 6=critical, 7=alert, 8=emerg, 9=off", 0 );

        fs << "LogLevel" << m_LogLevel;
        fs << "CudaEnabledDeviceCount" << m_CudaEnabledDeviceCount;
        fs << "CudaDeviceIndex" << m_CudaDeviceIndex;
        fs << "UseCuda" << m_UseCuda;
        fs << "}";

        // Save state for each filter
        for (int i = 0; i < m_Filters.size(); i++) {
            Processor filter = m_Filters[i];
            m_GraphData.m_Logger->info() << filter->m_FilterName;
            fs << filter->m_FilterName.c_str() << "{";
            // Persist the filter data
            filter->saveConfig(fs, m_GraphData);
            fs << "}";
        }
        fs.release();
    }

    void GraphManager::loadConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::READ);
        if (!fs.isOpened()) { m_GraphData.m_Logger->error() << "ERROR: unable to open file storage!" << m_persistFile; return; }

        auto node = fs["GraphManager"];
        node["CudaDeviceIndex"] >> m_CudaDeviceIndex;

        if (!node["LogLevel"].empty()) {
            node["LogLevel"] >> m_LogLevel;
        }
        if (!node["UseCuda"].empty()) {
            node["UseCuda"] >> m_UseCuda;
        }

        for (int i = 0; i < m_Filters.size(); i++) {
            Processor filter = m_Filters[i];
            auto node = fs[filter->m_FilterName.c_str()];
            if (!node.empty()) {
                filter->loadConfig(node, m_GraphData);
            }
        }
        fs.release();
    }
}
