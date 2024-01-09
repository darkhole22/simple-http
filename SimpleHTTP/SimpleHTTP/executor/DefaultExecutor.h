#pragma once
#include <SimpleHTTP/http.h>

#include <optional>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <stop_token>
#include <condition_variable>

namespace simpleHTTP {

class DefaultExecutor
{
public:
    DefaultExecutor();

    /// @brief 
    /// @note This function has effect only when called the first time.
    /// @param server 
    void run(HttpServer& server);

    void stop();

    ~DefaultExecutor();
private:
    std::stop_source m_StopSource;

    std::mutex m_StateMutex;
    bool m_Started = false;
    u32 m_MinThread = 1;
    u32 m_MaxThread = 0;
    std::vector<std::jthread> m_Threads;

    std::optional<HttpServerConnection> m_StagedConnection;
    std::mutex m_StagedConnectionMutex;
    std::condition_variable m_StagedConnectionCV;

    void setup();

    void processConnectionsImpl();
    static void processConnections(std::stop_token threadStopToken,
        std::stop_token executorStopToken,
        DefaultExecutor* executor);
};

}
