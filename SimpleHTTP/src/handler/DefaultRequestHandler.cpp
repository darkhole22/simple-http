#include <SimpleHTTP/handler/DefaultRequestHandler.h>

#include <iostream>

namespace simpleHTTP {

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

DefaultRequestHandler::DefaultRequestHandler() {}

bool DefaultRequestHandler::processRequest(const HttpRequest& request, HttpResponse& response) {
    response.setVersion(HttpVersion::V1_0);

    HttpMethod method = request.getMethod();
    const URI& uri = request.getURI();

    std::cout << std::format("Http Request: {} {} {}", request.getVersion(), method, uri) << std::endl;

    // for (auto& fieldLine : request.getAllHeaderFields()) {
    //     std::cout << std::format("\t{}: {}", fieldLine.first, fieldLine.second) << std::endl;
    // }

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
