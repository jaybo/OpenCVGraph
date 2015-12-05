
#include "stdafx.h"

namespace spd = spdlog;

namespace openCVGraph
{
    class GraphManager;
    typedef bool(*GraphCallback)(GraphManager* graphManager);
    typedef std::shared_ptr < openCVGraph::Filter> Processor;

    // Keep a vector of Filters and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    class  GraphManager {
    public:
        enum GraphState {
            Stop,
            Pause,      // Can only "step" in the Pause state
            Run
        };

    GraphManager(const std::string name, int primaryImageType, bool abortOnESC, GraphCallback callback)
    {
        // Set up logging
        try
        {
            string logDir = "logs";
            createDir(logDir);
            std::vector<spdlog::sink_ptr> sinks;
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
            sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(logDir + "/" + name + "logfile", "txt", 23, 59));
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
        Processor filter;
        ProcessResult result = ProcessResult::OK;

        // first process keyhits
        if (key != -1) {
            for (int i = 0; i < m_Filters.size(); i++) {
                filter = m_Filters[i];
                if (filter->IsEnabled()) {
                    fOK &= filter->processKeyboard(m_GraphData, key);
                }
            }
        }

        // MAKE ONE PASS THROUGH THE GRAPH
        for (int i = 0; i < m_Filters.size(); i++) {
            filter = m_Filters[i];
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
        Processor filter;
        ProcessResult result;

        loadConfig();

        // Let the filters know whether or not to use Cuda
        m_GraphData.m_UseCuda = m_UseCuda;

        // Init everybody
        for (int i = 0; i < m_Filters.size(); i++) {
            filter = m_Filters[i];
            if (filter->IsEnabled()) {
                fOK &= filter->init(m_GraphData);
                if (!fOK) {
                    m_GraphData.m_Logger->error() << "ERROR: " + m_Filters[i]->m_CombinedName << " failed init()";
                }
            }
        }

        // main processing loop
        while (fOK) {
            // This should be the only waitKey() in the entire graph
            int key = cv::waitKey(1);
            if (m_GraphData.m_AbortOnESC && (key == 27)) {
                fOK = false;
                m_GraphState = GraphState::Stop;
                break;
            }

            if (m_GraphState != GraphState::Run)
            {
                std::unique_lock<std::mutex> lk(m_mtx);
                m_cv.wait_for(lk, std::chrono::milliseconds(30), [=]() {return m_GraphState == GraphState::Stop; });
            }

            switch (m_GraphState) {
            case GraphState::Stop:
                // Snooze.  But this should be a mutex or awaitable object
                //std::this_thread::sleep_for(std::chrono::milliseconds(33));
                break;
            case GraphState::Pause:
                if (m_Stepping) {
                    result = ProcessOne(key);
                    fOK = fOK && (result != ProcessResult::Abort);
                    {
                        std::unique_lock<std::mutex> lk(m_mtx);
                        m_CompletedStep = true;
                        m_Stepping = false;
                        m_cv.notify_all();
                    }
                }
                // Snooze.  But this should be a mutex or awaitable object
                //std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
            filter = m_Filters[i];
            if (filter->IsEnabled()) {
                filter->fini(m_GraphData);
            }
        }

        cv::destroyAllWindows();

        {
            std::unique_lock<std::mutex> lk(m_mtx);
            m_CompletedStep = true;
            m_Stepping = false;
            m_cv.notify_all();
        }
        return fOK;
    }

    bool GraphManager::Step()
    {
        std::unique_lock<std::mutex> lck(m_mtx);

        if (m_GraphState == GraphState::Pause)
        {
            m_CompletedStep = false;
            m_Stepping = true;
            m_cv.notify_all();
            return true;
        }
        return false;
    }

    bool GraphManager::GotoState(GraphState newState)
    {
        std::unique_lock<std::mutex> lck(m_mtx);

        m_GraphState = newState;
        m_cv.notify_all();
        return true;
    }

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

    GraphData& getGraphData() { return m_GraphData; }

    void GraphManager::saveConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::WRITE);
        if (!fs.isOpened()) { m_GraphData.m_Logger->error() << "ERROR: unable to open file storage!" << m_persistFile; return; }

        // Save state for the graph manager
        fs << "GraphManager" << "{";
        // Persist the filter data
        cvWriteComment((CvFileStorage *)*fs, "Log Levels: 0=trace, 1=debug, 2=info, 3=notice, 4=warn, 5=err, 6=critical, 7=alert, 8=emerg, 9=off", 0);

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

    std::mutex& getWaitMutex() { return m_mtx; }
    std::condition_variable& getConditionalVariable () { return m_cv; }
    bool CompletedStep() { return m_CompletedStep; }

    private:
        std::string m_Name;                 
        string m_persistFile;
        GraphCallback m_GraphCallback;

        std::thread thread;
        GraphState m_GraphState;
        bool m_Stepping = false;

        std::mutex m_mtx;
        std::condition_variable m_cv;           // 
        bool m_CompletedStep = false;           // Has the step finished?
        bool m_CompletedRun = false;            // Has the run finished?

        GraphData m_GraphData;                  // data package sent to all members of the graph

        std::vector<Processor> m_Filters;       // filters in the graph

        int m_CudaEnabledDeviceCount;
        int m_CudaDeviceIndex = 1;
        bool m_UseCuda = true;
        int m_LogLevel = spd::level::info;
        std::shared_ptr<spdlog::logger> m_Logger;

    };

}
