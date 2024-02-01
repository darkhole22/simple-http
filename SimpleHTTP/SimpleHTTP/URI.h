#pragma once
#include <SimpleHTTP/types.h>

#include <vector>
#include <string_view>
#include <string>
#include <format>
#include <compare>
#include <filesystem>
#include <tuple>

namespace simpleHTTP {

class URI
{
public:
    using StringRange = std::pair<u64, u64>;

    URI();
    explicit URI(std::string_view uri);

    URI(const URI& other) = default;
    URI(URI&& other) = default;

    std::vector<std::string_view> getSegments();
    const std::vector<std::string_view> getSegments() const;

    const std::string_view getSegmentsSection() const;

    std::string_view getQuery();
    const std::string_view getQuery() const;

    bool isSubURI(const URI& other) const;

    std::string toString() const;

    URI& operator=(const URI& other) = default;
    URI& operator=(URI&& other) = default;

    ~URI();

    friend struct std::less<URI>;
private:
    std::string m_Raw;

    StringRange m_Scheme;
    StringRange m_Authority;
    std::vector<StringRange> m_Segments;
    StringRange m_Query;
    StringRange m_Fragment;
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

std::filesystem::path operator/(const std::filesystem::path& path, const simpleHTTP::URI& uri);
