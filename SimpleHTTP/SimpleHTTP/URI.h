#pragma once
#include <SimpleHTTP/types.h>

#include <vector>
#include <string_view>
#include <string>
#include <format>
#include <compare>

namespace simpleHTTP {

class URI
{
public:
    URI();
    URI(std::string_view uri);
    URI(const URI& other) = default;
    URI(URI&& other) = default;

    inline bool isAbsolute() const { return m_IsAbsoluteURI; }

    std::vector<std::string>& getSegments();
    const std::vector<std::string>& getSegments() const;

    std::vector<std::string>& getParameters();
    const std::vector<std::string>& getParameters() const;

    std::string& getQuery();
    const std::string& getQuery() const;

    std::string toString() const;

    URI& operator=(const URI& other) = default;
    URI& operator=(URI&& other) = default;

    ~URI();

    friend struct std::less<URI>;
private:
    bool m_IsAbsoluteURI = false;
    std::vector<std::string> m_Segments;
    std::vector<std::string> m_Parameters;
    std::string m_Query;
};

}

template <>
struct std::formatter<simpleHTTP::URI> : std::formatter<std::string>
{
    auto format(const simpleHTTP::URI& uri, format_context& ctx) const {
        return formatter<string>::format(uri.toString(), ctx);
    }

    formatter() = default;
};

template <>
struct std::less<simpleHTTP::URI>
{
    bool operator()(const simpleHTTP::URI& a, const simpleHTTP::URI& b) const {
        if (a.m_Segments.size() != b.m_Segments.size()) {
            return a.m_Segments.size() < b.m_Segments.size();
        }

        for (size_t i = 0; i < a.m_Segments.size(); i++) {
            auto cmp = a.m_Segments[i] <=> b.m_Segments[i];
            if (std::is_eq(cmp)) {
                continue;
            }
            return std::is_lt(cmp);
        }

        return false;
    }
};
