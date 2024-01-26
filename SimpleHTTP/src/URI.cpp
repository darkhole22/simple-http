#include <SimpleHTTP/URI.h>
#include "fsm.h"

#include <array>
#include <stdexcept>
#include <ranges>

namespace simpleHTTP {

static constexpr bool isAlpha(i8 c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static constexpr bool isDigit(i8 c) {
    return c >= '0' && c <= '9';
}

static constexpr bool isSafe(i8 c) {
    return c == '$' || c == '-' || c == '_' || c == '.';
}

static constexpr bool isExtra(i8 c) {
    return c == '!' || c == '*' || c == '\'' || c == '(' || c == ')' || c == ',';
}

static constexpr bool isReserved(i8 c) {
    return c == ';' || c == '/' || c == '?' || c == ':' || c == '@' || c == '&' || c == '=' || c == '+';
}

static constexpr bool isCtr(i8 c) {
    return c == 127 || (c >= 0 && c <= 31);
}

static constexpr bool isHex(i8 c) {
    return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || isDigit(c);
}

static constexpr bool isUnsafe(i8 c) {
    return c == '\"' || c == '#' || c == '%' || c == '<' || c == '>' || c == ' ' || isCtr(c);
}

static constexpr bool isNational(i8 c) {
    return !(isAlpha(c) || isDigit(c) || isReserved(c) || isExtra(c) || isSafe(c) || isUnsafe(c));
}

static constexpr bool isUnreserved(i8 c) {
    return isAlpha(c) || isDigit(c) || isSafe(c) || isExtra(c) || isNational(c);
}

static constexpr bool isUChar(i8 c) {
    return isUnreserved(c);
}

static constexpr bool isPChar(i8 c) {
    return  c == ':' || c == '@' || c == '&' || c == '=' || c == '+' || isUChar(c);
}

class URIBuilder
{
public:
    URIBuilder(std::vector<std::string>& segments,
               std::vector<std::string>& parameters,
               std::string& query) :
        m_Segments(segments), m_Parameters(parameters), m_Query(query) {}

    void startSegment() {
        if (m_Segments.size() <= 0)
            m_Segments.emplace_back();

        if (m_Segments[m_Segments.size() - 1].size() > 0)
            m_Segments.emplace_back();
    }

    void buildSegment(i8 c) {
        if (m_Segments.size() <= 0)
            return;

        m_Segments[m_Segments.size() - 1].push_back(c);
    }

    void startParameter() {
        m_Parameters.emplace_back();
    }

    void buildParameter(i8 c) {
        if (m_Parameters.size() <= 0)
            return;

        m_Parameters[m_Parameters.size() - 1].push_back(c);
    }

    void buildQuery(i8 c) {
        m_Query.push_back(c);
    }
private:
    std::vector<std::string>& m_Segments;
    std::vector<std::string>& m_Parameters;
    std::string& m_Query;
};

enum class URIParserState
{
    S0,
    F_SEGMENT,
    SEGMENT,
    PARAMS,
    ESCAPE_SEGMENT_1,
    ESCAPE_SEGMENT_2,
    QUERY,
    ESCAPE_PARAMS_1,
    ESCAPE_PARAMS_2,
    LAST
};

using URIParser = ParserFSM<URIParserState, URIBuilder, i8>;

static URIParserState s0_function(URIBuilder* builder, i8 c) {
    if (c == '/') {
        return URIParserState::F_SEGMENT;
    }

    return URIParserState::LAST;
};

static URIParserState fSegment_function(URIBuilder* builder, i8 c) {
    if (isPChar(c)) {
        builder->startSegment();
        builder->buildSegment(c);
        return URIParserState::SEGMENT;
    }

    if (c == '%') {
        builder->startSegment();
        builder->buildSegment(c);
        return URIParserState::ESCAPE_SEGMENT_1;
    }

    if (c == ';') {
        builder->startParameter();
        return URIParserState::PARAMS;
    }

    if (c == '?') {
        return URIParserState::QUERY;
    }

    return URIParserState::LAST;
};

static URIParserState segment_function(URIBuilder* builder, i8 c) {
    if (isPChar(c)) {
        builder->buildSegment(c);
        return URIParserState::SEGMENT;
    }

    if (c == '%') {
        builder->buildSegment(c);
        return URIParserState::ESCAPE_SEGMENT_1;
    }

    if (c == '/') {
        builder->startSegment();
        return URIParserState::SEGMENT;
    }

    if (c == ';') {
        builder->startParameter();
        return URIParserState::PARAMS;
    }

    if (c == '?') {
        return URIParserState::QUERY;
    }

    return URIParserState::LAST;
};

static URIParserState params_function(URIBuilder* builder, i8 c) {
    if (isPChar(c) || c == '/') {
        builder->buildParameter(c);
        return URIParserState::PARAMS;
    }

    if (c == '%') {
        builder->buildParameter(c);
        return URIParserState::ESCAPE_PARAMS_1;
    }

    if (c == ';') {
        builder->startParameter();
        return URIParserState::PARAMS;
    }

    if (c == '?') {
        return URIParserState::QUERY;
    }

    return URIParserState::LAST;
};

static URIParserState escapeSegment1_function(URIBuilder* builder, i8 c) {
    if (isHex(c)) {
        builder->buildSegment(c);
        return URIParserState::ESCAPE_SEGMENT_2;
    }

    return URIParserState::LAST;
};

static URIParserState escapeSegment2_function(URIBuilder* builder, i8 c) {
    if (isHex(c)) {
        builder->buildSegment(c);
        return URIParserState::SEGMENT;
    }

    return URIParserState::LAST;
};

static URIParserState query_function(URIBuilder* builder, i8 c) {
    if (isUChar(c) || isReserved(c)) {
        builder->buildQuery(c);
        return URIParserState::QUERY;
    }

    return URIParserState::LAST;
};

static URIParserState escapeParams1_function(URIBuilder* builder, i8 c) {
    if (isHex(c)) {
        builder->buildParameter(c);
        return URIParserState::ESCAPE_PARAMS_2;
    }

    return URIParserState::LAST;
};

static URIParserState escapeParams2_function(URIBuilder* builder, i8 c) {
    if (isHex(c)) {
        builder->buildParameter(c);
        return URIParserState::PARAMS;
    }

    return URIParserState::LAST;
};

constexpr URIParser parser = {
    s0_function,
    fSegment_function,
    segment_function,
    params_function,
    escapeSegment1_function,
    escapeSegment2_function,
    query_function,
    escapeParams1_function,
    escapeParams2_function,
};

URI::URI() {}

URI::URI(std::string_view uri) {
    URIBuilder builder(m_Segments, m_Parameters, m_Query);

    if (!parser.parse(&builder, uri, URIParserState::S0)) {
        throw std::runtime_error("Invalid URI!");
    }
}

std::vector<std::string>& URI::getSegments() {
    return m_Segments;
}

const std::vector<std::string>& URI::getSegments() const {
    return m_Segments;
}

std::vector<std::string>& URI::getParameters() {
    return m_Parameters;
}

const std::vector<std::string>& URI::getParameters() const {
    return m_Parameters;
}

std::string& URI::getQuery() {
    return m_Query;
}

const std::string& URI::getQuery() const {
    return m_Query;
}

bool URI::isSubURI(const URI& other) const {
    if (other.m_Segments.size() < m_Segments.size()) {
        return false;
    }

    auto segment = m_Segments.cbegin();
    auto otherSegment = other.m_Segments.cbegin();

    for (; segment != m_Segments.cend() && otherSegment != other.m_Segments.cend(); ++segment, ++otherSegment) {
        if (*segment != *otherSegment) {
            return false;
        }
    }

    return true;
}

std::string URI::toString() const {
    std::string result;

    for (auto& segment : m_Segments) {
        result.append("/");
        result.append(segment);
    }

    if (result.size() == 0) {
        result.append("/");
    }

    return result;
}

URI::~URI() {}

}

std::filesystem::path operator/(const std::filesystem::path& path, const simpleHTTP::URI& uri) {
    std::string s = uri.toString();

    if (s.size() <= 1) {
        return path;
    }

    std::string_view view(s.data() + 1, s.size() - 1);
    return path / std::filesystem::path(view);
}
