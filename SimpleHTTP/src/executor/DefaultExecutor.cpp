#include <SimpleHTTP/executor/DefaultExecutor.h>

#include <algorithm>
#include <iterator>
#include <iostream>

namespace simpleHTTP {

DefaultExecutor::DefaultExecutor() {}

void DefaultExecutor::run(HttpServer& server) {
    {
        std::scoped_lock lk(m_StateMutex);

        if (m_Started) {
            return;
        }

        m_Started = true;
    }

    setup();

    while (!m_StopSource.stop_requested()) {
        std::optional<HttpServerConnection> connection = std::nullopt;

        try {
            connection = server.accept();
        } catch (const std::exception&) {
            m_StagedConnectionCV.notify_all();
            break;
        }

        {
            std::unique_lock lk(m_StagedConnectionMutex);
            m_StagedConnectionCV.wait(lk, [this] {
                return !m_StagedConnection.has_value();
            });
        }

        {
            std::lock_guard lk(m_StagedConnectionMutex);
            m_StagedConnection = connection;
        }
        m_StagedConnectionCV.notify_one();
    }

    stop();
}

void DefaultExecutor::stop() {
    m_StopSource.request_stop();
}

void DefaultExecutor::setup() {
    std::scoped_lock lk(m_StateMutex);

    if (m_Threads.size() < static_cast<std::size_t>(m_MinThread)) {
        m_Threads.reserve(m_MinThread);
        std::generate_n(std::back_inserter(m_Threads),
                        static_cast<std::size_t>(m_MinThread) - m_Threads.size(),
                        [this] {
            return std::jthread(processConnections, m_StopSource.get_token(), this);
        });
    }
}

void DefaultExecutor::processConnections(std::stop_token threadStopToken, std::stop_token executorStopToken, DefaultExecutor* executor) {
    while (!(threadStopToken.stop_requested() || executorStopToken.stop_requested())) {
        executor->processConnectionsImpl();
    }
}

void DefaultExecutor::processConnectionsImpl() {
    std::optional<HttpServerConnection> connection = std::nullopt;
    {
        std::unique_lock lk(m_StagedConnectionMutex);
        m_StagedConnectionCV.wait(lk, [this] {
            return m_StagedConnection.has_value() || m_StopSource.stop_requested();
        });

        if (m_StopSource.stop_requested()) {
            return;
        }

        m_StagedConnection.swap(connection);
    }
    m_StagedConnectionCV.notify_all();

    bool keepAlive = false;
    do {
        try {
            HttpRequest request = connection->getNextRequest();
            HttpResponse response = connection->makeResponse();

            if (m_ProcessRequest) {
                try {
                    if (m_ProcessRequest(request, response)) {
                        // Success
                        continue;
                    }
                } catch (...) {}
                // TODO: Implement handling logic.
                // Failure
            }
            else {
                // Not Implemented
            }
        } catch (const std::exception& ex) {
            // TODO: Proper Logging
            std::cout << ex.what() << std::endl;
            keepAlive = false;
        } catch (...) {
            // TODO: Proper Logging
            std::cout << "Unrecognized Exception" << std::endl;
            keepAlive = false;
        }
    } while (keepAlive);

    connection->close();
}

DefaultExecutor::~DefaultExecutor() {
    stop();
}

} // namespace simpleHTTP
