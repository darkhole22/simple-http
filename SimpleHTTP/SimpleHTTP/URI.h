#pragma once
#include <SimpleHTTP/types.h>

#include <vector>
#include <string_view>
#include <string>

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

    URI& operator=(const URI& other) = default;
    URI& operator=(URI&& other) = default;

    ~URI();
private:
    bool m_IsAbsoluteURI = false;
    std::vector<std::string> m_Segments;
    std::vector<std::string> m_Parameters;
    std::string m_Query;
};

}
