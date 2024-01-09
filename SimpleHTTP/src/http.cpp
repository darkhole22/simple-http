#include <SimpleHTTP/http.h>
#include <iostream>

namespace simpleHTTP {

HttpServerConnection::HttpServerConnection(ClientSocket&& socket)
    : m_Socket(socket) {}

HttpServerConnection::~HttpServerConnection() {}

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

} // namespace simpleHTTP

