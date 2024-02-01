#include <SimpleHTTP/handler/DefaultRequestHandler.h>

#include <iostream>
#include <array>
#include <numeric>

namespace simpleHTTP {

RequestProcessor::RequestProcessor(InitializerList processors)
    : m_ProcessFunctions(processors.begin(), processors.end()) {}

std::string RequestProcessor::getMethodsList() const {
    auto it = m_ProcessFunctions.begin();
    auto end = m_ProcessFunctions.end();

    if (it == end)
        return std::string();

    std::string result = httpMethodToString(it->first);

    ++it;
    for (; it != end; ++it) {
        result.append(", ");
        result.append(httpMethodToString(it->first));
    }

    return result;
}

std::unique_ptr<Resource> RequestProcessor::operator()(const HttpRequest& request) const {
    HttpMethod method = request.getMethod();

    auto it = m_ProcessFunctions.find(method);

    if (it != m_ProcessFunctions.end()) {
        return it->second(request);
    }

    return std::make_unique<Resource>();
}

void DefaultRequestHandlerSettings::registerRequestProcessor(std::string_view uri,
    RequestProcessor::InitializerList processors) {
    m_RequestProcessors.emplace(uri, processors);
}

DefaultRequestHandler::DefaultRequestHandler(const DefaultRequestHandlerSettings& settings)
    : m_HttpVersion(settings.httpVersion), m_RequestProcessors(settings.m_RequestProcessors) {}

bool DefaultRequestHandler::processRequest(const HttpRequest& request, HttpResponse& response) const {
    if (m_RequestFilter && !m_RequestFilter(request)) {
        return false;
    }

    response.setVersion(m_HttpVersion);

    if (request.getVersion().major > m_HttpVersion.major) {
        response.setStatusCode(StatusCode::HTTP_VERSION_NOT_SUPPORTED);
        return true;
    }

    HttpMethod method = request.getMethod();
    const URI& uri = request.getURI();

    if (m_RequestProcessors.size() == 0) {
        return false;
    }

    const RequestProcessor* requestProcessor = nullptr;
    {
        auto it = m_RequestProcessors.crbegin();
        for (; it != m_RequestProcessors.crend(); ++it) {
            if (it->first.isSubURI(uri)) {
                requestProcessor = &it->second;
                break;
            }
        }

        if (it == m_RequestProcessors.crend()) {
            return false;
        }
    }

    if (requestProcessor == nullptr) {
        return false;
    }

    auto resource = (*requestProcessor)(request);
    response.setStatusCode(resource->getStatusCode());

    response.addHeaderField("Allow", requestProcessor->getMethodsList());

    response.addHeaderField("Cache-Control", "no-cache");
    response.addHeaderField("X-Content-Type-Options", "nosniff");

    u64 contentLength = resource->getContentLength();

    if (contentLength == 0) {
        return true;
    }

    {
        std::array<char, 22> contentLengthS{};
        auto [ptr, ec] = std::to_chars(contentLengthS.data(),
            contentLengthS.data() + contentLengthS.size(),
            contentLength);
        if (ec != std::errc()) {
            return false;
        }

        response.addHeaderField("Content-Length", std::string_view(contentLengthS.data(), ptr));
    }

    ContentType contentType = resource->getContentType();
    if (contentType) {
        response.addHeaderField("Content-Type", contentType.toString());
    }

    response.send([&resource](ClientSocket* socket) {
        resource->sendCallback(socket);
    });

    return true;
}

DefaultRequestHandler::~DefaultRequestHandler() {}

} // namespace simpleHTTP
