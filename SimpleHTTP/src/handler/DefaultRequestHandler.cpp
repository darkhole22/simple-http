#include <SimpleHTTP/handler/DefaultRequestHandler.h>

#include <iostream>

namespace simpleHTTP {

RequestProcessor::RequestProcessor(InitializerList processors)
    : m_ProcessFunctions(processors.begin(), processors.end()) {}

Resource RequestProcessor::operator()(const HttpRequest& request) const {
    HttpMethod method = request.getMethod();

    auto it = m_ProcessFunctions.find(method);

    if (it != m_ProcessFunctions.end()) {
        return it->second(request);
    }

    return Resource{};
}

void DefaultRequestHandlerSettings::registerRequestProcessor(std::string_view uri,
                                                             RequestProcessor::InitializerList processors) {
    m_RequestProcessors.emplace(uri, processors);
}

static const std::string indexHTML =
"<!DOCTYPE html>"
"<html lang=\'en\'>"
"<head>"
"<meta charset=\'utf-8\'>"
"<title>Page Title</title>"
"<meta name=\'viewport\' content=\'width=device-width, initial-scale=1\'>"
"</head>"
"<body>"
"</body>"
"</html>";

DefaultRequestHandler::DefaultRequestHandler(const DefaultRequestHandlerSettings& settings)
    : m_HttpVersion(settings.httpVersion), m_RequestProcessors(settings.m_RequestProcessors) {}

bool DefaultRequestHandler::processRequest(const HttpRequest& request, HttpResponse& response) const {
    if (m_RequestFilter && !m_RequestFilter(request)) {
        return false;
    }

    response.setVersion(m_HttpVersion);

    if (isMajorHttpVersionGrater(request.getVersion(), m_HttpVersion)) {
        response.setStatusCode(StatusCode::HTTP_VERSION_NOT_SUPPORTED);
        return true;
    }

    HttpMethod method = request.getMethod();
    const URI& uri = request.getURI();

    if (m_RequestProcessors.size() == 0) {
        return false;
    }

    // Find best fit
    // If no fit retur falses

    auto resource = m_RequestProcessors.begin()->second(request);
    response.setStatusCode(resource.getStatusCode());

    response.addHeaderField("Allow", "GET, HEAD");

    if (method != HttpMethod::GET && method != HttpMethod::HEAD) {
        response.setStatusCode(StatusCode::NOT_IMPLEMENTED);
        return true;
    }

    if (uri.toString() == "/") {
        response.setStatusCode(StatusCode::OK);

        response.addHeaderField("Content-Length", std::format("{}", indexHTML.size()));
        response.addHeaderField("Content-Type", "text/html; charset=utf-8");
        response.addHeaderField("Cache-Control", "no-cache");
        response.addHeaderField("X-Content-Type-Options", "nosniff");

        response.send([](ClientSocket* socket) {
            socket->send(indexHTML.data(), indexHTML.size());
        });
        return true;
    }

    response.setStatusCode(StatusCode::NOT_FOUND);

    return true;
}

DefaultRequestHandler::~DefaultRequestHandler() {}




} // namespace simpleHTTP
