#include <SimpleHTTP/http.h>
#include <iostream>

#include <array>
#include <ranges>
#include <cctype>
#include <algorithm>

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

        std::string_view fieldName(buffer.begin(), colon);

        auto beginField = std::find_if_not(colon + 1, end, [](i8 c) { return c == ' '; });
        std::string_view fieldValue(beginField, end);

        m_HeaderFields.emplace(std::make_pair(fieldName, fieldValue));
    } while (headerFieldLen > 0);
}

HttpVersion HttpRequest::getVersion() const {
    return m_Version;
}

HttpRequest::~HttpRequest() {}

HttpResponse::HttpResponse(ClientSocket* socket)
    : m_Socket(socket) {}

HttpResponse::~HttpResponse() {}

} // namespace simpleHTTP

