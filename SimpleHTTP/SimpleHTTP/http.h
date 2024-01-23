#pragma once
#include <SimpleHTTP/socket.h>
#include <SimpleHTTP/URI.h>

#include <format>
#include <functional>
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

enum class StatusCode : u16
{
    OK = 200,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501
};

struct HttpServerSettings
{
    u16 unused = 0;
};

class HttpServerConnection;

class HttpRequest
{
public:
    using HeaderFieldsMap = std::unordered_multimap<std::string, std::string>;

    HttpVersion getVersion() const;

    HttpMethod getMethod() const;

    const URI& getURI() const;

    void setHeaderField(std::string_view name, std::string_view value);
    void addHeaderField(std::string_view name, std::string_view value);

    inline auto getHeaderFields(std::string_view name) const {
        return m_HeaderFields.equal_range(getFieldNameIgnoreCase(name));
    }

    const HeaderFieldsMap& getAllHeaderFields() const;

    ~HttpRequest();

    friend class HttpServerConnection;
private:
    ClientSocket* m_Socket;
    HttpVersion m_Version = HttpVersion::UNKNOWN;
    HttpMethod m_Method = HttpMethod::UNKNOWN;
    URI m_Uri;
    HeaderFieldsMap m_HeaderFields;

    HttpRequest(ClientSocket* socket);

    std::string getFieldNameIgnoreCase(std::string_view name) const;
};

class HttpResponse
{
public:
    ~HttpResponse();

    void setVersion(HttpVersion version);

    void setStatusCode(StatusCode code);
    void setStatusCode(u16 code);

    void setReasonPhrase(std::string_view reason);
    void generateDefaultReasonPhrase();

    void setUseDefaultReasonPhrase(bool v);

    void addHeaderField(std::string_view name, std::string_view value);
    void clearHeaderFields();

    bool wasSent() const;

    void send();
    void send(std::function<void(ClientSocket*)> body);

    friend class HttpServerConnection;
private:
    ClientSocket* m_Socket;
    HttpVersion m_Version = HttpVersion::UNKNOWN;
    u16 m_StatusCode = 500;
    bool m_UseDefaultReasonPhrase = true;
    bool m_WasSent = false;
    std::string m_ReasonPhrase;
    std::vector<std::pair<std::string, std::string>> m_HeaderFields;

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

    u16 getPort() const;

    void stop();

    ~HttpServer();
private:
    const HttpServerSettings m_Settings;
    ServerSocket m_Socket;
};

} // namespace simpleHTTP

template <>
struct std::formatter<simpleHTTP::HttpVersion> : std::formatter<std::string>
{
    auto format(simpleHTTP::HttpVersion v, format_context& ctx) const {
        switch (v) {
        case simpleHTTP::HttpVersion::UNKNOWN:
        return formatter<string>::format("HTTP/0.0", ctx);

        case simpleHTTP::HttpVersion::V0_9:
        return formatter<string>::format("HTTP/0.9", ctx);

        case simpleHTTP::HttpVersion::V1_0:
        return formatter<string>::format("HTTP/1.0", ctx);

        case simpleHTTP::HttpVersion::V1_1:
        return formatter<string>::format("HTTP/1.1", ctx);

        case simpleHTTP::HttpVersion::V2_0:
        return formatter<string>::format("HTTP/2.0", ctx);

        case simpleHTTP::HttpVersion::V3_0:
        return formatter<string>::format("HTTP/3.0", ctx);

        default:
        return formatter<string>::format("HTTP/0.0", ctx);
        }
    }

    formatter() = default;
};

template <>
struct std::formatter<simpleHTTP::HttpMethod> : std::formatter<std::string>
{
    auto format(simpleHTTP::HttpMethod v, format_context& ctx) const {
        switch (v) {
        case simpleHTTP::HttpMethod::UNKNOWN:
        return formatter<string>::format("UNKNOWN", ctx);

        case simpleHTTP::HttpMethod::GET:
        return formatter<string>::format("GET", ctx);

        case simpleHTTP::HttpMethod::HEAD:
        return formatter<string>::format("HEAD", ctx);

        case simpleHTTP::HttpMethod::POST:
        return formatter<string>::format("POST", ctx);

        default:
        return formatter<string>::format("UNKNOWN", ctx);
        }
    }

    formatter() = default;
};
