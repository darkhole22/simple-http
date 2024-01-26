#pragma once
#include <SimpleHTTP/http.h>

#include <concepts>

namespace simpleHTTP {

class ContentType
{
public:
    std::string toString();
private:
};

class Resource
{
public:
    virtual inline StatusCodeType getStatusCode() const {
        return static_cast<StatusCodeType>(StatusCode::NOT_IMPLEMENTED);
    }

    virtual inline ContentType getContentType() const {
        return ContentType();
    }

    virtual inline u64 getContentLength() const {
        return 0;
    }

    virtual void sendCallback(ClientSocket* socket) {

    }

    virtual inline ~Resource() {

    }
private:
};

}
