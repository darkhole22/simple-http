#pragma once
#include <SimpleHTTP/http.h>
#include <SimpleHTTP/handler/Resource.h>

#include <map>

namespace simpleHTTP {

class RequestProcessor
{
public:
    using ProcessFunction = std::function<Resource(const HttpRequest&)>;
    using InitializerList = std::initializer_list<std::pair<HttpMethod, ProcessFunction>>;

    RequestProcessor(InitializerList processors);

    Resource operator()(const HttpRequest& request) const;
private:
    std::unordered_map<HttpMethod, ProcessFunction> m_ProcessFunctions;
};

class DefaultRequestHandlerSettings
{
public:
    HttpVersion httpVersion = HttpVersion::V1_0;

    void registerRequestProcessor(std::string_view uri, RequestProcessor::InitializerList processors);

    friend class DefaultRequestHandler;
private:
    std::map<URI, RequestProcessor> m_RequestProcessors;
};

class DefaultRequestHandler
{
public:
    explicit DefaultRequestHandler(const DefaultRequestHandlerSettings& settings);
    DefaultRequestHandler(const DefaultRequestHandler&) = delete;

    bool processRequest(const HttpRequest& request, HttpResponse& response) const;

    DefaultRequestHandler& operator=(const DefaultRequestHandler&) = delete;

    ~DefaultRequestHandler();
private:
    const HttpVersion m_HttpVersion;

    const std::function<bool(const HttpRequest&)> m_RequestFilter;
    const std::map<URI, RequestProcessor> m_RequestProcessors;

};

} // namespace simpleHTTP
