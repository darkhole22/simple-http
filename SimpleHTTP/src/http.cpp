#include <SimpleHTTP/http.h>
#include <iostream>

#include <array>
#include <ranges>
#include <cctype>
#include <algorithm>
#include <format>
#include <span>

namespace simpleHTTP {

static constexpr const u64 MAX_METHOD_LENGTH = 0xff;
static constexpr const u64 MAX_ELEMENT_LENGTH = 0x2000;
static constexpr const u64 MAX_VERSION_LENGTH = 0xff;

static constexpr const std::array<i8, 2> CRLF = { 13, 10 };
static constexpr const i8 SP = 32;
static constexpr const i8 HTAB = 9;

const HttpVersion HttpVersion::UNKNOWN{ 0,0 };
const HttpVersion HttpVersion::V0_9{ 0,9 };
const HttpVersion HttpVersion::V1_0{ 1,0 };
const HttpVersion HttpVersion::V1_1{ 1,1 };
const HttpVersion HttpVersion::V2_0{ 2,0 };
const HttpVersion HttpVersion::V3_0{ 3,0 };

bool HttpVersion::operator==(const HttpVersion& other) const noexcept {
    return other.major == major && other.minor == minor;
}

static constexpr bool ignoreCaseEquals(std::string_view lhs, std::string_view rhs) {
    return std::ranges::equal(lhs, rhs, [](u8 a, u8 b) {
        return std::tolower(a) == std::tolower(b);
    });
}

static constexpr bool useCaseEquals(std::string_view lhs, std::string_view rhs) {
    return lhs == rhs;
}

static constexpr const auto toLowerView = std::views::transform([](i8 c) -> i8 { return std::tolower(c); });

static constexpr HttpMethod getMethodFromString(std::string_view str) {
    if (ignoreCaseEquals(str, "GET")) {
        return HttpMethod::GET;
    }
    if (ignoreCaseEquals(str, "HEAD")) {
        return HttpMethod::HEAD;
    }
    if (ignoreCaseEquals(str, "POST")) {
        return HttpMethod::POST;
    }
    if (ignoreCaseEquals(str, "PUT")) {
        return HttpMethod::PUT;
    }
    if (ignoreCaseEquals(str, "DELETE")) {
        return HttpMethod::DELETE;
    }
    if (ignoreCaseEquals(str, "CONNECT")) {
        return HttpMethod::CONNECT;
    }
    if (ignoreCaseEquals(str, "OPTIONS")) {
        return HttpMethod::OPTIONS;
    }
    if (ignoreCaseEquals(str, "TRACE")) {
        return HttpMethod::TRACE;
    }
    return HttpMethod::UNKNOWN;
}

static constexpr HttpVersion getVersionFromString(std::string_view str) {
    if (str.size() < 8) {
        return HttpVersion::UNKNOWN;
    }

    if (!(str[0] == 'H' && str[1] == 'T' && str[2] == 'T' && str[3] == 'P' && str[4] == '/')) {
        return HttpVersion::UNKNOWN;
    }

    auto beginMajor = str.begin() + 5;
    auto endMajor = beginMajor;
    for (; endMajor != str.end() && *endMajor != '.'; ++endMajor) {

    }
    std::string_view major{ beginMajor , endMajor };
    std::string_view minor{ endMajor + 1, str.end() };

    HttpVersion result{};

    if (std::from_chars(major.data(), major.data() + major.size(), result.major).ec != std::errc()) {
        return HttpVersion::UNKNOWN;
    }

    if (std::from_chars(minor.data(), minor.data() + minor.size(), result.minor).ec != std::errc()) {
        return HttpVersion::UNKNOWN;
    }

    return result;
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
    // TODO: use custom allocator?
    std::vector<i8> buffer{};
    buffer.resize(MAX_METHOD_LENGTH + MAX_ELEMENT_LENGTH + MAX_VERSION_LENGTH + 2);

    u64 requestLineLength = m_Socket->receiveUntil(buffer.data(), buffer.size(), CRLF.data(), CRLF.size());
    if (requestLineLength == 0) {
        // In the interest of robustness, a server that is expecting to receive and parse
        // a request-line *SHOULD* ignore at least one empty line (CRLF) received prior to 
        // the request-line.
        // 
        // https://datatracker.ietf.org/doc/html/rfc9112#section-2.2-6
        requestLineLength = m_Socket->receiveUntil(buffer.data(), buffer.size(), CRLF.data(), CRLF.size());
    }

    auto methodEnd = std::find(buffer.begin(), buffer.end(), SP);
    if (methodEnd == buffer.end()) {
        throw std::runtime_error("Invalid request line.");
    }
    std::string_view method(buffer.begin(), methodEnd);

    auto uriEnd = std::find(methodEnd + 1, buffer.end(), SP);
    if (uriEnd == buffer.end()) {
        throw std::runtime_error("Invalid request line.");
    }
    std::string uri(methodEnd + 1, uriEnd);

    std::string_view version(uriEnd + 1, buffer.end());
    m_Version = getVersionFromString(version);

    if (m_Version == HttpVersion::UNKNOWN || m_Version.major > 1) {
        throw std::runtime_error(std::format("Invalid version detected {}.", m_Version));
    }

    m_Method = getMethodFromString(method);
    if (m_Method == HttpMethod::UNKNOWN) {
        // TODO implement bad request exception
        throw std::runtime_error(std::format("Invalid method detected {}.", method));
    }

    u64 headerFieldLen = 0;
    do {
        headerFieldLen = m_Socket->receiveUntil(buffer.data(), buffer.size(), CRLF.data(), CRLF.size());
        if (headerFieldLen == 0)
            break;

        auto end = buffer.begin() + headerFieldLen;
        auto colon = std::find(buffer.begin(), end, ':');

        constexpr auto isWhiteSpace = [](i8 c) {
            return c == SP || c == HTAB;
        };

        auto whiteSpaceFieldName = std::find_if(buffer.begin(), colon, isWhiteSpace);

        if (whiteSpaceFieldName != colon) {
            // TODO implement bad request exception
            throw std::runtime_error("Error while parsing Header fields.");
        }

        if (colon == end)
            continue;

        auto fieldName = std::string_view(buffer.begin(), colon) | toLowerView;

        std::ranges::subrange fieldValue{ colon + 1, end };
        auto beginField = std::find_if_not(fieldValue.begin(), fieldValue.end(), isWhiteSpace);
        auto endField = fieldValue.end();

        for (; endField != beginField && isWhiteSpace(*beginField); --endField);

        if (std::distance(beginField, endField) == 0) {
            // TODO implement bad request exception
            throw std::runtime_error("Error while parsing Header fields.");
        }

        m_HeaderFields.emplace(std::piecewise_construct,
            std::make_tuple(fieldName.begin(), fieldName.end()),
            std::make_tuple(beginField, endField));
    } while (headerFieldLen > 0);

    auto hostsRange = m_HeaderFields.equal_range("host");

    if (std::distance(hostsRange.first, hostsRange.second) != 1) {
        // TODO implement bad request exception
        throw std::runtime_error("A valid Request must contain exactly one 'Host' field.");
    }

    try
    {
        m_Uri = URI(hostsRange.first->second + uri);
    }
    catch (...) {
        // TODO implement bad request exception
        throw std::runtime_error("Error while parsing the request uri.");
    }
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

    StatusCode code = static_cast<StatusCode>(m_StatusCode);

    switch (code) {
    case simpleHTTP::StatusCode::CONTINUE: m_ReasonPhrase = "Continue";
        break;
    case simpleHTTP::StatusCode::SWITCHING_PROTOCOLS: m_ReasonPhrase = "Switching Protocols";
        break;
    case simpleHTTP::StatusCode::OK: m_ReasonPhrase = "OK";
        break;
    case simpleHTTP::StatusCode::CREATED: m_ReasonPhrase = "Created";
        break;
    case simpleHTTP::StatusCode::ACCEPTED: m_ReasonPhrase = "Accepted";
        break;
    case simpleHTTP::StatusCode::NON_AUTHORITATIVE_INFORMATION: m_ReasonPhrase = "Non-Authoritative Information";
        break;
    case simpleHTTP::StatusCode::NO_CONTENT: m_ReasonPhrase = "No Content";
        break;
    case simpleHTTP::StatusCode::RESET_REQUEST: m_ReasonPhrase = "Reset Content";
        break;
    case simpleHTTP::StatusCode::PARTIAL_CONTENT: m_ReasonPhrase = "Partial Content";
        break;
    case simpleHTTP::StatusCode::MULTIPLE_CHOICES: m_ReasonPhrase = "Multiple Choices";
        break;
    case simpleHTTP::StatusCode::MOVED_PERMANENTLY: m_ReasonPhrase = "Moved Permanently";
        break;
    case simpleHTTP::StatusCode::FOUND: m_ReasonPhrase = "Found";
        break;
    case simpleHTTP::StatusCode::SEE_OTHER: m_ReasonPhrase = "See Other";
        break;
    case simpleHTTP::StatusCode::NOT_MODIFIED: m_ReasonPhrase = "Not Modified";
        break;
    case simpleHTTP::StatusCode::USE_PROXY: m_ReasonPhrase = "Use Proxy";
        break;
    case simpleHTTP::StatusCode::TEMPORARY_REDIRECT: m_ReasonPhrase = "Temporary Redirect";
        break;
    case simpleHTTP::StatusCode::PERMANENT_REDIRECT: m_ReasonPhrase = "Permanent Redirect";
        break;
    case simpleHTTP::StatusCode::BAD_REQUEST: m_ReasonPhrase = "Bad Request";
        break;
    case simpleHTTP::StatusCode::UNAUTHORIZED: m_ReasonPhrase = "Unauthorized";
        break;
    case simpleHTTP::StatusCode::PAYMENT_REQUIRED: m_ReasonPhrase = "Payment Required";
        break;
    case simpleHTTP::StatusCode::FORBIDDEN: m_ReasonPhrase = "Forbidden";
        break;
    case simpleHTTP::StatusCode::NOT_FOUND: m_ReasonPhrase = "Not Found";
        break;
    case simpleHTTP::StatusCode::METHOD_NOT_ALLOWED: m_ReasonPhrase = "Method Not Allowed";
        break;
    case simpleHTTP::StatusCode::NOT_ACCEPTABLE: m_ReasonPhrase = "Not Acceptable";
        break;
    case simpleHTTP::StatusCode::PROXY_AUTHENTICATION_REQUIRED: m_ReasonPhrase = "Proxy Authentication Required";
        break;
    case simpleHTTP::StatusCode::REQUEST_TIMEOUT: m_ReasonPhrase = "Request Timeout";
        break;
    case simpleHTTP::StatusCode::CONFLICT: m_ReasonPhrase = "Conflict";
        break;
    case simpleHTTP::StatusCode::GONE: m_ReasonPhrase = "Gone";
        break;
    case simpleHTTP::StatusCode::LENGTH_REQUIRED: m_ReasonPhrase = "Length Required";
        break;
    case simpleHTTP::StatusCode::PRECONDITION_FAILED: m_ReasonPhrase = "Precondition Failed";
        break;
    case simpleHTTP::StatusCode::CONTENT_TOO_LARGE: m_ReasonPhrase = "Content Too Large";
        break;
    case simpleHTTP::StatusCode::URI_TOO_LONG: m_ReasonPhrase = "URI Too Long";
        break;
    case simpleHTTP::StatusCode::UNSUPPORTED_MEDIA_TYPE: m_ReasonPhrase = "Unsupported Media Type";
        break;
    case simpleHTTP::StatusCode::RANGE_NOT_SATISFIABLE: m_ReasonPhrase = "Range Not Satisfiable";
        break;
    case simpleHTTP::StatusCode::EXPECTATION_FAILED: m_ReasonPhrase = "Expectation Failed";
        break;
    case simpleHTTP::StatusCode::I_AM_A_TEAPOT: m_ReasonPhrase = "I'm a Teapot";
        break;
    case simpleHTTP::StatusCode::MISDIRECTED_REQUEST: m_ReasonPhrase = "Misdirected Request";
        break;
    case simpleHTTP::StatusCode::UNPROCESSABLE_CONTENT: m_ReasonPhrase = "Unprocessable Content";
        break;
    case simpleHTTP::StatusCode::UPGRADE_REQUIRED: m_ReasonPhrase = "Upgrade Required";
        break;
    case simpleHTTP::StatusCode::INTERNAL_SERVER_ERROR: m_ReasonPhrase = "Internal Server Error";
        break;
    case simpleHTTP::StatusCode::NOT_IMPLEMENTED: m_ReasonPhrase = "Not Implemented";
        break;
    case simpleHTTP::StatusCode::BAD_GATEWAY: m_ReasonPhrase = "Bad Gateway";
        break;
    case simpleHTTP::StatusCode::SERVICE_UNAVAILABLE: m_ReasonPhrase = "Service Unavailable";
        break;
    case simpleHTTP::StatusCode::GATEWAY_TIMEOUT: m_ReasonPhrase = "Gateway Timeout";
        break;
    case simpleHTTP::StatusCode::HTTP_VERSION_NOT_SUPPORTED: m_ReasonPhrase = "HTTP Version Not Supported";
        break;
    default:
        break;
    }
}

const char* httpMethodToString(HttpMethod m) {
    switch (m) {
    case simpleHTTP::HttpMethod::UNKNOWN:
        return "UNKNOWN";

    case simpleHTTP::HttpMethod::GET:
        return "GET";

    case simpleHTTP::HttpMethod::HEAD:
        return "HEAD";

    case simpleHTTP::HttpMethod::POST:
        return "POST";

    case simpleHTTP::HttpMethod::PUT:
        return "PUT";

    case simpleHTTP::HttpMethod::DELETE:
        return "DELETE";

    case simpleHTTP::HttpMethod::CONNECT:
        return "CONNECT";

    case simpleHTTP::HttpMethod::OPTIONS:
        return "OPTIONS";

    case simpleHTTP::HttpMethod::TRACE:
        return "TRACE";

    default:
        break;
    }

    return "UNKNOWN";
}

} // namespace simpleHTTP

