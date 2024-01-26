#pragma once
#include <SimpleHTTP/socket.h>
#include <SimpleHTTP/URI.h>

#include <format>
#include <functional>
#include <unordered_map>

namespace simpleHTTP {

enum class HttpVersion : u32
{
    UNKNOWN,
    V0_9 = 900,
    V1_0 = 1000,
    V1_1 = 1100,
    V2_0 = 2000,
    V3_0 = 3000
};

bool isMajorHttpVersionGrater(HttpVersion v1, HttpVersion v2);

enum class HttpMethod
{
    UNKNOWN,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
};

const char* httpMethodToString(HttpMethod m);

using StatusCodeType = u16;

enum class StatusCode : StatusCodeType
{
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_REQUEST = 205,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    CONTENT_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    I_AM_A_TEAPOT = 418,
    MISDIRECTED_REQUEST = 421,
    UNPROCESSABLE_CONTENT = 422,
    UPGRADE_REQUIRED = 426,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SEVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
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
    void setStatusCode(StatusCodeType code);

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
    StatusCodeType m_StatusCode = 500;
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
        return formatter<string>::format(simpleHTTP::httpMethodToString(v), ctx);
    }

    formatter() = default;
};
