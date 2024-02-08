#include <SimpleHTTP/URI.h>

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
    URIBuilder(
        URI::StringRange& scheme,
        URI::StringRange& authority,
        std::vector<URI::StringRange>& segments,
        URI::StringRange& query,
        URI::StringRange& fragment)
        : m_Scheme(scheme), m_Authority(authority), m_Segments(segments),
        m_Query(query), m_Fragment(fragment) {}

    bool build(std::string_view input);

    constexpr bool hasNext() {
        return head != end;
    }

    constexpr [[nodiscard]] i8 peek() {
        return *head;
    }

    template <typename Pred>
    constexpr [[nodiscard]] bool peek(Pred f) {
        return f(*head);
    }

    constexpr i8 pop() {
        return *(head++);
    }

    constexpr u64 getOffset() {
        return std::distance(input.begin(), head);
    }

    constexpr void appendSegment(u64 first, u64 second) {
        std::string_view segment(input.begin() + first, input.begin() + second);

        if (segment.size() == 0 || segment == ".")
            return;

        if (segment == "..") {
            if (m_Segments.size() > 0) {
                m_Segments.pop_back();
            }
            return;
        }
        m_Segments.emplace_back(first, second);
    }

private:
    URI::StringRange& m_Scheme;
    URI::StringRange& m_Authority;
    std::vector<URI::StringRange>& m_Segments;
    URI::StringRange& m_Query;
    URI::StringRange& m_Fragment;

    std::string_view input;
    std::string_view::const_iterator head;
    std::string_view::const_iterator end;
};

bool URIBuilder::build(std::string_view _input) {
    input = _input;
    head = std::begin(input);
    end = std::end(input);

    if (!hasNext())
        return false;

    bool isAbempty = false;
    auto parseAuthority = [&isAbempty, this]() {
        // parse authority
        pop();
        u64 beginAuthority = getOffset();
        while (hasNext() && peek() != '/') {
            pop();
            // TODO: Validate authority? Authority class?
        }

        if (!hasNext())
            return false;

        m_Authority = { beginAuthority, getOffset() };

        isAbempty = true;
        return true;
    };

    if (peek(isAlpha)) {
        u64 beginScheme = getOffset();
        pop();

        while (hasNext() && peek([](i8 c) {
            return isAlpha(c) || isDigit(c) || c == '+' || c == '-' || c == '.';
        })) {
            pop();
        }

        if (!hasNext())
            return false;

        m_Scheme = { beginScheme, getOffset() };

        if (pop() != ':')
            return false;

        if (!hasNext()) {
            // path-empty with no query and no fragment
            return true;
        }

        if (peek() == '/') {
            pop();
            // "//" authority path-abempty or path-absolute 

            if (!hasNext())
                return false;

            if (peek() == '/') {
                if (!parseAuthority())
                    return false;
            }
        }
    }
    else {
        if (hasNext() && peek() == '/')
            isAbempty = true;

        if (hasNext() && peek() != '/' && !parseAuthority())
            return false;
    }

    auto isFragmentChar = [](i8 c) {
        return isPChar(c) || c == '%';
    };

    if (!isAbempty) {
        // parse segment_nz
        if (!hasNext() || !peek(isFragmentChar))
            return false;

        if (peek() == '%') {
            pop();
            if (!hasNext() || !peek(isHex))
                return false;
            pop();
            if (!hasNext() || !peek(isHex))
                return false;
        }

        u64 beginSegmentNz = getOffset();
        pop();

        while (hasNext() && peek(isFragmentChar)) {
            if (peek() == '%') {
                pop();
                if (!hasNext() || !peek(isHex))
                    return false;
                pop();
                if (!hasNext() || !peek(isHex))
                    return false;
            }
            pop();
        }

        if (hasNext() && peek() != '/')
            return false;

        appendSegment(beginSegmentNz, getOffset());
    }

    // *( "/" segment )
    while (hasNext() && peek() == '/')
    {
        // /abcd/abcd/
        pop();
        u64 beginSegment = getOffset();
        while (hasNext() && peek(isFragmentChar)) {
            if (peek() == '%') {
                pop();
                if (!hasNext() || !peek(isHex))
                    return false;
                pop();
                if (!hasNext() || !peek(isHex))
                    return false;
            }
            pop();
        }

        appendSegment(beginSegment, getOffset());

        if (hasNext() && peek() != '/')
            break;
    }

    // pase query
    if (hasNext() && peek() == '?') {
        pop();
        u64 beginQuery = getOffset();
        while (hasNext() && peek([](i8 c) {
            return isPChar(c) || c == '/' || c == '?';
        })) {
            pop();
        }

        m_Query = { beginQuery, getOffset() };
    }

    // parse fragment
    if (hasNext() && peek() != '#')
        return false;

    if (!hasNext())
        return true;

    pop();

    u64 beginFragment = getOffset();
    while (hasNext() && peek([](i8 c) {
        return isPChar(c) || c == '/' || c == '?';
    })) {
        pop();
    }

    if (hasNext())
        return false;

    m_Fragment = { beginFragment, getOffset() };
    return true;
}

URI::URI() {}

URI::URI(std::string_view uri) {
    URIBuilder builder(m_Scheme, m_Authority, m_Segments, m_Query, m_Fragment);

    if (!builder.build(uri)) {
        throw std::runtime_error("Invalid URI!");
    }
    m_Raw.reserve(uri.size());

    auto getSubstr = [uri](StringRange view) {
        return uri.substr(view.first, view.second - view.first);
    };

    auto getSize = [](StringRange view) {
        return view.second - view.first;
    };

    if (getSize(m_Scheme) > 0) {
        m_Raw.append(getSubstr(m_Scheme));
        m_Raw.push_back(':');
    }

    m_Raw.append(getSubstr(m_Authority));

    if (m_Segments.size() == 0) {
        m_Raw.push_back('/');
    }

    for (auto& segment : m_Segments) {
        m_Raw.push_back('/');
        u64 first = m_Raw.size();
        m_Raw.append(getSubstr(segment));

        segment.first = first;
        segment.second = m_Raw.size();
    }

    if (getSize(m_Query) > 0) {
        m_Raw.push_back('?');
        u64 first = m_Raw.size();
        m_Raw.append(getSubstr(m_Query));

        m_Query.first = first;
        m_Query.second = m_Raw.size();
    }

    if (getSize(m_Fragment) > 0) {
        m_Raw.push_back('#');
        u64 first = m_Raw.size();
        m_Raw.append(getSubstr(m_Fragment));

        m_Fragment.first = first;
        m_Fragment.second = m_Raw.size();
    }

    m_Raw.shrink_to_fit();
}

std::vector<std::string_view> URI::getSegments() {
    std::vector<std::string_view> segments;
    for (auto& segment : m_Segments) {
        segments.emplace_back(m_Raw.begin() + segment.first, m_Raw.begin() + segment.second);
    }
    return segments;
}

const std::vector<std::string_view> URI::getSegments() const {
    std::vector<std::string_view> segments;
    for (auto& segment : m_Segments) {
        segments.emplace_back(m_Raw.begin() + segment.first, m_Raw.begin() + segment.second);
    }
    return segments;
}

const std::string_view URI::getSegmentsSection() const
{
    if (m_Segments.size() == 0)
        return std::string_view();

    auto first = m_Segments.front().first;
    auto last = m_Segments.back().second;

    return std::string_view(m_Raw.begin() + first, m_Raw.begin() + last);
}

std::string_view URI::getQuery() {
    return std::string_view(m_Raw.begin() + m_Query.first, m_Raw.begin() + m_Query.second);
}

const std::string_view URI::getQuery() const {
    return std::string_view(m_Raw.begin() + m_Query.first, m_Raw.begin() + m_Query.second);
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
    return m_Raw;
}

URI::~URI() {}

}

std::filesystem::path operator/(const std::filesystem::path& path, const simpleHTTP::URI& uri) {
    auto segment = uri.getSegmentsSection();

    if (segment.size() < 1) {
        return path;
    }

    return path / segment;
}
