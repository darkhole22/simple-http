#pragma once
#include <SimpleHTTP/socket.h>

namespace simpleHTTP {

struct HttpServerSettings
{
    u16 unused = 0;
};

class HttpServerConnection
{
public:
    void close();

    ~HttpServerConnection();

    friend class HttpServer;
private:
    ClientSocket m_Socket;

    HttpServerConnection(ClientSocket&& socket);
};

class HttpServer
{
public:
    HttpServer(HttpServerSettings config);

    HttpServerConnection accept();

    void stop();

    ~HttpServer();
private:
    const HttpServerSettings m_Settings;
    ServerSocket m_Socket;
};

} // namespace simpleHTTP

