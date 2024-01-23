#pragma once
#include <SimpleHTTP/socket.h>
#include <SimpleHTTP/URI.h>

#include <unordered_map>

namespace simpleHTTP {

enum class HttpVersion
{
    UNKNOWN,
    V0_9,
    V1_0,
    V1_1,
    V2_0,
    V3_0
};

enum class HttpMethod
{
    UNKNOWN,
    GET,
    HEAD,
    POST
};

struct HttpServerSettings
{
    u16 unused = 0;
};

class HttpServerConnection;

class HttpRequest
{
public:
    HttpVersion getVersion() const;

    ~HttpRequest();

    friend class HttpServerConnection;
private:
    ClientSocket* m_Socket;
    HttpVersion m_Version = HttpVersion::UNKNOWN;
    HttpMethod m_Method = HttpMethod::UNKNOWN;
    URI m_Uri;
    std::unordered_multimap<std::string, std::string> m_HeaderFields;

    HttpRequest(ClientSocket* socket);
};

class HttpResponse
{
public:
    ~HttpResponse();

    friend class HttpServerConnection;
private:
    ClientSocket* m_Socket;

    HttpResponse(ClientSocket* socket);
};

class HttpServerConnection
{
public:
    HttpRequest getNextRequest();
    HttpResponse makeResponse();

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

