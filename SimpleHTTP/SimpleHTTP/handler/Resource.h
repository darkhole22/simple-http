#pragma once
#include <SimpleHTTP/http.h>

#include <concepts>

namespace simpleHTTP {

class ContentType
{
public:
    
private:
};

template<class Res>
concept ResourceType = requires(Res res) {
    { true };
}&& requires(const Res res) {
    { res.getStatusCode() } -> std::convertible_to<StatusCodeType>;
};

class Resource
{
public:
    inline Resource()
        : m_StatusCode(static_cast<StatusCodeType>(StatusCode::INTERNAL_SERVER_ERROR)) {}

    Resource(const Resource& res) = delete;

    template<ResourceType Res>
    Resource(const Res& res)
        : m_StatusCode(static_cast<StatusCodeType>(res.getStatusCode())) {

    }

    inline StatusCodeType getStatusCode() const {
        return m_StatusCode;
    }

    Resource& operator=(const Resource& other) = delete;
private:
    StatusCodeType m_StatusCode;
};

}
