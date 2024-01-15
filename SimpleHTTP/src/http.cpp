#include <SimpleHTTP/http.h>
#include <iostream>

namespace simpleHTTP {

HttpServerConnection::HttpServerConnection(ClientSocket&& socket)
    : m_Socket(socket) {}

HttpServerConnection::~HttpServerConnection() {}

HttpRequest HttpServerConnection::getNextRequest() {
    return HttpRequest(&m_Socket);
}

HttpResponse HttpServerConnection::makeResponse() {
    return HttpResponse(&m_Socket);
}

void HttpServerConnection::close() {
    m_Socket.close();
}

HttpServer::HttpServer(HttpServerSettings config)
    : m_Settings(std::move(config)), m_Socket(8001) {}

HttpServerConnection HttpServer::accept() {
    return { std::move(m_Socket.accept()) };
}

void HttpServer::stop() {
    m_Socket.close();
}

HttpServer::~HttpServer() {

}

HttpRequest::HttpRequest(ClientSocket* socket)
    : m_Socket(socket) {
}

HttpVersion HttpRequest::getVersion() const {
    return m_Version;
}

HttpRequest::~HttpRequest() {}

HttpResponse::HttpResponse(ClientSocket* socket)
    : m_Socket(socket) {
}

HttpResponse::~HttpResponse() {}

} // namespace simpleHTTP

