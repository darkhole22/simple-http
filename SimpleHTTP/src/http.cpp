#include <SimpleHTTP/http.h>
#include <iostream>

#include <array>
#include <ranges>
#include <cctype>
#include <algorithm>
#include <format>

namespace simpleHTTP {

static constexpr const u64 MAX_ELEMENT_LENGTH = 0x2000;

static constexpr const std::array<i8, 2> CRLF = { '\r', '\n' };
static constexpr const std::array<i8, 1> SP = { ' ' };

static constexpr bool ignoreCaseEquals(std::string_view lhs, std::string_view rhs) {
    return std::ranges::equal(lhs, rhs, [](u8 a, u8 b) {
        return std::tolower(a) == std::tolower(b);
    });
}

static constexpr bool useCaseEquals(std::string_view lhs, std::string_view rhs) {
    return lhs == rhs;
}

static constexpr const auto toLowerView = std::views::transform([](i8 c) -> i8 { return std::tolower(c); });

static constexpr HttpMethod FromString(std::string_view str) {
    if (ignoreCaseEquals(str, "GET")) {
        return HttpMethod::GET;
    }
    if (ignoreCaseEquals(str, "HEAD")) {
        return HttpMethod::HEAD;
    }
    if (ignoreCaseEquals(str, "POST")) {
        return HttpMethod::POST;
    }
    return HttpMethod::UNKNOWN;
}

static constexpr HttpVersion getVersionFromString(std::string_view str) {
    if (useCaseEquals(str, "HTTP/0.9")) {
        return HttpVersion::V0_9;
    }
    if (useCaseEquals(str, "HTTP/1.0")) {
        return HttpVersion::V1_0;
    }
    if (useCaseEquals(str, "HTTP/1.1")) {
        return HttpVersion::V1_1;
    }
    if (useCaseEquals(str, "HTTP/2.0")) {
        return HttpVersion::V2_0;
    }
    if (useCaseEquals(str, "HTTP/3.0")) {
        return HttpVersion::V3_0;
    }
    return HttpVersion::UNKNOWN;
}

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

u16 HttpServer::getPort() const {
    return m_Socket.getPort();
}

void HttpServer::stop() {
    m_Socket.close();
}

HttpServer::~HttpServer() {

}

HttpRequest::HttpRequest(ClientSocket* socket)
    : m_Socket(socket) {
    std::array<i8, MAX_ELEMENT_LENGTH> buffer{};

    u64 methodLen = m_Socket->receiveUntil(buffer.data(), buffer.size(), SP.data(), SP.size());
    std::string_view method(buffer.begin(), buffer.begin() + methodLen);
    m_Method = FromString(method);

    u64 uriLen = m_Socket->receiveUntil(buffer.data(), buffer.size(), SP.data(), SP.size());
    std::string_view uri(buffer.begin(), buffer.begin() + uriLen);
    m_Uri = std::move(URI(uri));

    u64 versionLen = m_Socket->receiveUntil(buffer.data(), buffer.size(), CRLF.data(), CRLF.size());
    std::string_view version(buffer.begin(), buffer.begin() + versionLen);
    m_Version = getVersionFromString(version);

    u64 headerFieldLen = 0;
    do {
        headerFieldLen = m_Socket->receiveUntil(buffer.data(), buffer.size(), CRLF.data(), CRLF.size());

        auto end = buffer.begin() + headerFieldLen;
        auto colon = std::find(buffer.begin(), end, ':');

        if (colon == end)
            continue;

        auto fieldName = std::string_view(buffer.begin(), colon) | toLowerView;

        auto beginField = std::find_if_not(colon + 1, end, [](i8 c) { return c == ' '; });

        m_HeaderFields.emplace(std::piecewise_construct,
                               std::make_tuple(fieldName.begin(), fieldName.end()),
                               std::make_tuple(beginField, end));
    } while (headerFieldLen > 0);
}

std::string HttpRequest::getFieldNameIgnoreCase(std::string_view name) const {
    auto fieldName = name | toLowerView;
    return std::string(fieldName.begin(), fieldName.end());
}

HttpVersion HttpRequest::getVersion() const {
    return m_Version;
}

HttpMethod HttpRequest::getMethod() const {
    return m_Method;
}

const URI& HttpRequest::getURI() const {
    return m_Uri;
}

void HttpRequest::setHeaderField(std::string_view _name, std::string_view value) {
    auto fieldName = _name | toLowerView;
    std::string name(fieldName.begin(), fieldName.end());

    auto it = m_HeaderFields.find(name);

    if (it == m_HeaderFields.end()) {
        m_HeaderFields.emplace(std::piecewise_construct,
                               std::make_tuple(fieldName.begin(), fieldName.end()),
                               std::make_tuple(value));
        return;
    }

    it->second = value;
    // TODO remove duplicates?
}

const HttpRequest::HeaderFieldsMap& HttpRequest::getAllHeaderFields() const {
    return m_HeaderFields;
}

void HttpRequest::addHeaderField(std::string_view name, std::string_view value) {
    auto fieldName = name | toLowerView;
    m_HeaderFields.emplace(std::piecewise_construct,
                           std::make_tuple(fieldName.begin(), fieldName.end()),
                           std::make_tuple(value));
}

HttpRequest::~HttpRequest() {}

HttpResponse::HttpResponse(ClientSocket* socket)
    : m_Socket(socket) {}

HttpResponse::~HttpResponse() {}

void HttpResponse::setVersion(HttpVersion version) {
    m_Version = version;
}

void HttpResponse::setStatusCode(StatusCode code) {
    setStatusCode(static_cast<StatusCodeType>(code));
}

void HttpResponse::setStatusCode(StatusCodeType code) {
    if (code < 100 || code > 999)
        return;

    m_StatusCode = code;
}

void HttpResponse::setReasonPhrase(std::string_view reason) {
    m_UseDefaultReasonPhrase = false;
    m_ReasonPhrase = reason;
}

void HttpResponse::setUseDefaultReasonPhrase(bool v) {
    m_UseDefaultReasonPhrase = v;
}

void HttpResponse::addHeaderField(std::string_view name, std::string_view value) {
    m_HeaderFields.emplace_back(name, value);
}

void HttpResponse::clearHeaderFields() {
    m_HeaderFields.clear();
}

bool HttpResponse::wasSent() const {
    return m_WasSent;
}

void HttpResponse::send() {
    send(nullptr);
}

void HttpResponse::send(std::function<void(ClientSocket*)> body) {
    if (m_WasSent)
        return;

    m_WasSent = true;

    if (m_UseDefaultReasonPhrase) {
        generateDefaultReasonPhrase();
    }

    auto statusLine = std::format("{} {} {}\r\n", m_Version, m_StatusCode, m_ReasonPhrase);

    m_Socket->send(statusLine.data(), statusLine.size());
    for (auto& headerField : m_HeaderFields) {
        auto field = std::format("{}: {}\r\n", headerField.first, headerField.second);
        m_Socket->send(field.data(), field.size());
    }
    m_Socket->send("\r\n", 2);

    if (body) {
        body(m_Socket);
    }
}

void HttpResponse::generateDefaultReasonPhrase() {
    m_UseDefaultReasonPhrase = true;
    switch (m_StatusCode) {
    case 200: m_ReasonPhrase = "OK";
        break;
    case 201: m_ReasonPhrase = "Created";
        break;
    case 202: m_ReasonPhrase = "Accepted";
        break;
    case 204: m_ReasonPhrase = "No Content";
        break;
    case 301: m_ReasonPhrase = "Moved Permanently";
        break;
    case 302: m_ReasonPhrase = "Moved Temporarily";
        break;
    case 304: m_ReasonPhrase = "Not Modified";
        break;
    case 400: m_ReasonPhrase = "Bad Request";
        break;
    case 401: m_ReasonPhrase = "Unauthorized";
        break;
    case 403: m_ReasonPhrase = "Forbidden";
        break;
    case 404: m_ReasonPhrase = "Not Found";
        break;
    case 500: m_ReasonPhrase = "Internal Server Error";
        break;
    case 501: m_ReasonPhrase = "Not Implemented";
        break;
    case 502: m_ReasonPhrase = "Bad Gateway";
        break;
    case 503: m_ReasonPhrase = "Service Unavailable";
        break;
    default:
    break;
    }
}

bool isMajorHttpVersionGrater(HttpVersion _v1, HttpVersion _v2) {
    u32 v1 = static_cast<u32>(_v1);
    u32 v2 = static_cast<u32>(_v2);

    return (v1 / 1000) > (v2 / 1000);
}

} // namespace simpleHTTP

