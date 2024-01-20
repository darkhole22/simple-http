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

    ~URI();
private:
    bool m_IsAbsoluteURI = false;
    std::vector<std::string> m_Segments;
    std::vector<std::string> m_Parameters;
    std::string m_Query;
};

}
